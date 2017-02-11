#include <RendererCore/PCH.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpotLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezSpotLightComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(0.0f), new ezSuffixAttribute(" m"), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("InnerSpotAngle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(179.0f)), new ezDefaultValueAttribute(ezAngle::Degree(15.0f))),
    EZ_ACCESSOR_PROPERTY("OuterSpotAngle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(179.0f)), new ezDefaultValueAttribute(ezAngle::Degree(30.0f))),
    //EZ_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  }
  EZ_END_PROPERTIES
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSpotLightVisualizerAttribute("OuterSpotAngle", "Range", "Intensity", "LightColor"),
    new ezConeManipulatorAttribute("OuterSpotAngle", "Range"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezSpotLightComponent::ezSpotLightComponent()
  : m_fRange(0.0f)
  , m_InnerSpotAngle(ezAngle::Degree(15.0f))
  , m_OuterSpotAngle(ezAngle::Degree(30.0f))
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

ezSpotLightComponent::~ezSpotLightComponent()
{

}

ezResult ezSpotLightComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  const float t = ezMath::Tan(m_OuterSpotAngle * 0.5f);
  const float p = ezMath::Min(t * m_fEffectiveRange, m_fEffectiveRange);

  ezBoundingBox box;
  box.m_vMin = ezVec3(0.0f, -p, -p);
  box.m_vMax = ezVec3(m_fEffectiveRange, p, p);

  bounds = box;
  return EZ_SUCCESS;
}

void ezSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate(true);
}

float ezSpotLightComponent::GetRange() const
{
  return m_fRange;
}

void ezSpotLightComponent::SetInnerSpotAngle(ezAngle spotAngle)
{
  m_InnerSpotAngle = ezMath::Clamp(spotAngle, ezAngle::Degree(0.0f), m_OuterSpotAngle);
}

ezAngle ezSpotLightComponent::GetInnerSpotAngle() const
{
  return m_InnerSpotAngle;
}

void ezSpotLightComponent::SetOuterSpotAngle(ezAngle spotAngle)
{
  m_OuterSpotAngle = ezMath::Clamp(spotAngle, m_InnerSpotAngle, ezAngle::Degree(179.0f));

  TriggerLocalBoundsUpdate(true);
}

ezAngle ezSpotLightComponent::GetOuterSpotAngle() const
{
  return m_OuterSpotAngle;
}

void ezSpotLightComponent::SetProjectedTexture(const ezTexture2DResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;
}

const ezTexture2DResourceHandle& ezSpotLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void ezSpotLightComponent::SetProjectedTextureFile(const char* szFile)
{
  ezTexture2DResourceHandle hProjectedTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = ezResourceManager::LoadResource<ezTexture2DResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* ezSpotLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  return m_hProjectedTexture.GetResourceID();
}

void ezSpotLightComponent::OnExtractRenderData( ezExtractRenderDataMessage& msg ) const
{
  if (msg.m_OverrideCategory != ezInvalidIndex)
    return;

  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezSpotLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_InnerSpotAngle = m_InnerSpotAngle;
  pRenderData->m_OuterSpotAngle = m_OuterSpotAngle;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_bCastShadows = m_bCastShadows;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId);
}

void ezSpotLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fRange;
  s << m_InnerSpotAngle;
  s << m_OuterSpotAngle;
  s << GetProjectedTextureFile();
}

void ezSpotLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fRange;
  s >> m_InnerSpotAngle;
  s >> m_OuterSpotAngle;

  ezStringBuilder temp;
  s >> temp;
  SetProjectedTextureFile(temp);
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpotLightVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezSpotLightVisualizerAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSpotLightVisualizerAttribute::ezSpotLightVisualizerAttribute()
  : ezVisualizerAttribute(nullptr)
{
}

ezSpotLightVisualizerAttribute::ezSpotLightVisualizerAttribute(const char* szAngleProperty, const char* szRangeProperty, const char* szIntensityProperty, const char* szColorProperty)
  : ezVisualizerAttribute(szAngleProperty, szRangeProperty, szIntensityProperty, szColorProperty)
{
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class ezSpotLightComponentPatch_1_2 : public ezGraphPatch
{
public:
  ezSpotLightComponentPatch_1_2()
    : ezGraphPatch(ezGetStaticRTTI<ezSpotLightComponent>(), 2) {}

  virtual void Patch(ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    PatchBaseClass(pGraph, pNode, ezGetStaticRTTI<ezLightComponent>(), 2);

    pNode->RenameProperty("Inner Spot Angle", "InnerSpotAngle");
    pNode->RenameProperty("Outer Spot Angle", "OuterSpotAngle");
  }
};

ezSpotLightComponentPatch_1_2 g_ezSpotLightComponentPatch_1_2;


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);

