#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDirectionalLightRenderData, 1, ezRTTIDefaultAllocator<ezDirectionalLightRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezDirectionalLightComponent, 3, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("NumCascades", GetNumCascades, SetNumCascades)->AddAttributes(new ezClampValueAttribute(1, 4), new ezDefaultValueAttribute(2)),
    EZ_ACCESSOR_PROPERTY("MinShadowRange", GetMinShadowRange, SetMinShadowRange)->AddAttributes(new ezClampValueAttribute(0.1f, ezVariant()), new ezDefaultValueAttribute(30.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new ezClampValueAttribute(0.6f, 1.0f), new ezDefaultValueAttribute(0.8f)),
    EZ_ACCESSOR_PROPERTY("SplitModeWeight", GetSplitModeWeight, SetSplitModeWeight)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.7f)),
    EZ_ACCESSOR_PROPERTY("NearPlaneOffset", GetNearPlaneOffset, SetNearPlaneOffset)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(100.0f), new ezSuffixAttribute(" m")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 1.0f, ezColor::White, "LightColor"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezDirectionalLightComponent::ezDirectionalLightComponent() = default;
ezDirectionalLightComponent::~ezDirectionalLightComponent() = default;

ezResult ezDirectionalLightComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  ref_bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezDirectionalLightComponent::SetNumCascades(ezUInt32 uiNumCascades)
{
  m_uiNumCascades = ezMath::Clamp(uiNumCascades, 1u, 4u);

  InvalidateCachedRenderData();
}

ezUInt32 ezDirectionalLightComponent::GetNumCascades() const
{
  return m_uiNumCascades;
}

void ezDirectionalLightComponent::SetMinShadowRange(float fMinShadowRange)
{
  m_fMinShadowRange = ezMath::Max(fMinShadowRange, 0.0f);

  InvalidateCachedRenderData();
}

float ezDirectionalLightComponent::GetMinShadowRange() const
{
  return m_fMinShadowRange;
}

void ezDirectionalLightComponent::SetFadeOutStart(float fFadeOutStart)
{
  m_fFadeOutStart = ezMath::Clamp(fFadeOutStart, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float ezDirectionalLightComponent::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void ezDirectionalLightComponent::SetSplitModeWeight(float fSplitModeWeight)
{
  m_fSplitModeWeight = ezMath::Clamp(fSplitModeWeight, 0.0f, 1.0f);

  InvalidateCachedRenderData();
}

float ezDirectionalLightComponent::GetSplitModeWeight() const
{
  return m_fSplitModeWeight;
}

void ezDirectionalLightComponent::SetNearPlaneOffset(float fNearPlaneOffset)
{
  m_fNearPlaneOffset = ezMath::Max(fNearPlaneOffset, 0.0f);

  InvalidateCachedRenderData();
}

float ezDirectionalLightComponent::GetNearPlaneOffset() const
{
  return m_fNearPlaneOffset;
}

void ezDirectionalLightComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezDirectionalLightRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? ezShadowPool::AddDirectionalLight(this, msg.m_pView) : ezInvalidIndex;

  pRenderData->FillBatchIdAndSortingKey(1.0f);

  ezRenderData::Caching::Enum caching = m_bCastShadows ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, caching);
}

void ezDirectionalLightComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_uiNumCascades;
  s << m_fMinShadowRange;
  s << m_fFadeOutStart;
  s << m_fSplitModeWeight;
  s << m_fNearPlaneOffset;
}

void ezDirectionalLightComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  if (uiVersion >= 3)
  {
    s >> m_uiNumCascades;
    s >> m_fMinShadowRange;
    s >> m_fFadeOutStart;
    s >> m_fSplitModeWeight;
    s >> m_fNearPlaneOffset;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class ezDirectionalLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezDirectionalLightComponentPatch_1_2()
    : ezGraphPatch("ezDirectionalLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& ref_context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    ref_context.PatchBaseClass("ezLightComponent", 2, true);
  }
};

ezDirectionalLightComponentPatch_1_2 g_ezDirectionalLightComponentPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);
