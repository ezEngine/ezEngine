#include <RendererCorePCH.h>

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
    EZ_ACCESSOR_PROPERTY("NumCascades", GetNumCascades, SetNumCascades)->AddAttributes(new ezClampValueAttribute(1, 4), new ezDefaultValueAttribute(3)),
    EZ_ACCESSOR_PROPERTY("MinShadowRange", GetMinShadowRange, SetMinShadowRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(50.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("FadeOutStart", GetFadeOutStart, SetFadeOutStart)->AddAttributes(new ezClampValueAttribute(0.6f, 1.0f), new ezDefaultValueAttribute(0.8f)),
    EZ_ACCESSOR_PROPERTY("SplitModeWeight", GetSplitModeWeight, SetSplitModeWeight)->AddAttributes(new ezClampValueAttribute(0.0f, 1.0f), new ezDefaultValueAttribute(0.7f)),
    EZ_ACCESSOR_PROPERTY("NearPlaneOffset", GetNearPlaneOffset, SetNearPlaneOffset)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(100.0f), new ezSuffixAttribute(" m")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezDirectionVisualizerAttribute(ezBasisAxis::PositiveX, 0.75f, "LightColor"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezDirectionalLightComponent::ezDirectionalLightComponent()
    : m_uiNumCascades(3)
    , m_fMinShadowRange(50.0f)
    , m_fFadeOutStart(0.8f)
    , m_fSplitModeWeight(0.7f)
    , m_fNearPlaneOffset(100.0f)
{
}

ezDirectionalLightComponent::~ezDirectionalLightComponent() {}

ezResult ezDirectionalLightComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  bAlwaysVisible = true;
  return EZ_SUCCESS;
}

void ezDirectionalLightComponent::SetNumCascades(ezUInt32 uiNumCascades)
{
  m_uiNumCascades = ezMath::Clamp(uiNumCascades, 1u, 4u);
}

ezUInt32 ezDirectionalLightComponent::GetNumCascades() const
{
  return m_uiNumCascades;
}

void ezDirectionalLightComponent::SetMinShadowRange(float fMinShadowRange)
{
  m_fMinShadowRange = ezMath::Max(fMinShadowRange, 0.0f);
}

float ezDirectionalLightComponent::GetMinShadowRange() const
{
  return m_fMinShadowRange;
}

void ezDirectionalLightComponent::SetFadeOutStart(float fFadeOutStart)
{
  m_fFadeOutStart = ezMath::Clamp(fFadeOutStart, 0.0f, 1.0f);
}

float ezDirectionalLightComponent::GetFadeOutStart() const
{
  return m_fFadeOutStart;
}

void ezDirectionalLightComponent::SetSplitModeWeight(float fSplitModeWeight)
{
  m_fSplitModeWeight = ezMath::Clamp(fSplitModeWeight, 0.0f, 1.0f);
}

float ezDirectionalLightComponent::GetSplitModeWeight() const
{
  return m_fSplitModeWeight;
}

void ezDirectionalLightComponent::SetNearPlaneOffset(float fNearPlaneOffset)
{
  m_fNearPlaneOffset = ezMath::Max(fNearPlaneOffset, 0.0f);
}

float ezDirectionalLightComponent::GetNearPlaneOffset() const
{
  return m_fNearPlaneOffset;
}

void ezDirectionalLightComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f)
    return;

  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezDirectionalLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? ezShadowPool::AddDirectionalLight(this, msg.m_pView) : ezInvalidIndex;

  ezRenderData::Caching::Enum caching = m_bCastShadows ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId, caching);
}

void ezDirectionalLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_uiNumCascades;
  s << m_fMinShadowRange;
  s << m_fFadeOutStart;
  s << m_fSplitModeWeight;
  s << m_fNearPlaneOffset;
}

void ezDirectionalLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

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

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezDirectionalLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezDirectionalLightComponentPatch_1_2()
      : ezGraphPatch("ezDirectionalLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.PatchBaseClass("ezLightComponent", 2, true);
  }
};

ezDirectionalLightComponentPatch_1_2 g_ezDirectionalLightComponentPatch_1_2;



EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);

