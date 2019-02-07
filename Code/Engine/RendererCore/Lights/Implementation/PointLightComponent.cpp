#include <RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPointLightRenderData, 1, ezRTTIDefaultAllocator<ezPointLightRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezPointLightComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(0.0f), new ezSuffixAttribute(" m"), new ezMinValueTextAttribute("Auto")),
    //EZ_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture Cube")),
  }
  EZ_END_PROPERTIES;
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
    EZ_BEGIN_ATTRIBUTES
  {
    new ezSphereManipulatorAttribute("Range"),
    new ezPointLightVisualizerAttribute("Range", "Intensity", "LightColor"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezPointLightComponent::ezPointLightComponent()
    : m_fRange(0.0f)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

ezPointLightComponent::~ezPointLightComponent() {}

ezResult ezPointLightComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  bounds = ezBoundingSphere(ezVec3::ZeroVector(), m_fEffectiveRange);
  return EZ_SUCCESS;
}

void ezPointLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float ezPointLightComponent::GetRange() const
{
  return m_fRange;
}

float ezPointLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void ezPointLightComponent::SetProjectedTexture(const ezTextureCubeResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;
}

const ezTextureCubeResourceHandle& ezPointLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void ezPointLightComponent::SetProjectedTextureFile(const char* szFile)
{
  ezTextureCubeResourceHandle hProjectedTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = ezResourceManager::LoadResource<ezTextureCubeResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* ezPointLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  return m_hProjectedTexture.GetResourceID();
}

void ezPointLightComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  ezTransform t = GetOwner()->GetGlobalTransform();

  float fScreenSpaceSize =
      CalculateScreenSpaceSize(ezBoundingSphere(t.m_vPosition, m_fEffectiveRange * 0.5f), *msg.m_pView->GetCullingCamera());

  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;
  auto pRenderData = ezCreateRenderDataForThisFrame<ezPointLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = t;
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? ezShadowPool::AddPointLight(this, fScreenSpaceSize) : ezInvalidIndex;

  ezRenderData::Caching::Enum caching = m_bCastShadows ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId, caching);
}

void ezPointLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fRange;
  s << m_hProjectedTexture;
}

void ezPointLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fRange;
  s >> m_hProjectedTexture;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPointLightVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezPointLightVisualizerAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezPointLightVisualizerAttribute::ezPointLightVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
}

ezPointLightVisualizerAttribute::ezPointLightVisualizerAttribute(const char* szRangeProperty, const char* szIntensityProperty,
                                                                 const char* szColorProperty)
    : ezVisualizerAttribute(szRangeProperty, szIntensityProperty, szColorProperty)
{
}

  //////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezPointLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezPointLightComponentPatch_1_2()
      : ezGraphPatch("ezPointLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.PatchBaseClass("ezLightComponent", 2, true);
  }
};

ezPointLightComponentPatch_1_2 g_ezPointLightComponentPatch_1_2;

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);

