#include <PCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
ezCVarBool CVarVisLightSize("r_VisLightScreenSpaceSize", false, ezCVarFlags::Default,
                            "Enables debug visualization of light screen space size calculation");
#endif

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpotLightRenderData, 1, ezRTTIDefaultAllocator<ezSpotLightRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezSpotLightComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(0.0f), new ezSuffixAttribute(" m"), new ezMinValueTextAttribute("Auto")),
    EZ_ACCESSOR_PROPERTY("InnerSpotAngle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(179.0f)), new ezDefaultValueAttribute(ezAngle::Degree(15.0f))),
    EZ_ACCESSOR_PROPERTY("OuterSpotAngle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(179.0f)), new ezDefaultValueAttribute(ezAngle::Degree(30.0f))),
    //EZ_ACCESSOR_PROPERTY("ProjectedTexture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D;Render Target")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezSpotLightVisualizerAttribute("OuterSpotAngle", "Range", "Intensity", "LightColor"),
    new ezConeLengthManipulatorAttribute("Range"),
    new ezConeAngleManipulatorAttribute("OuterSpotAngle", 1.5f),
    new ezConeAngleManipulatorAttribute("InnerSpotAngle", 1.5f),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSpotLightComponent::ezSpotLightComponent()
    : m_fRange(0.0f)
    , m_InnerSpotAngle(ezAngle::Degree(15.0f))
    , m_OuterSpotAngle(ezAngle::Degree(30.0f))
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);
}

ezSpotLightComponent::~ezSpotLightComponent() {}

ezResult ezSpotLightComponent::GetLocalBounds(ezBoundingBoxSphere& bounds, bool& bAlwaysVisible)
{
  m_fEffectiveRange = CalculateEffectiveRange(m_fRange, m_fIntensity);

  bounds = CalculateBoundingSphere(ezTransform::IdentityTransform(), m_fEffectiveRange);
  return EZ_SUCCESS;
}

void ezSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  TriggerLocalBoundsUpdate();
}

float ezSpotLightComponent::GetRange() const
{
  return m_fRange;
}

float ezSpotLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
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

  TriggerLocalBoundsUpdate();
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

void ezSpotLightComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't extract light render data for selection or in shadow views.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f || m_OuterSpotAngle.GetRadian() <= 0.0f)
    return;

  ezTransform t = GetOwner()->GetGlobalTransform();
  ezBoundingSphere bs = CalculateBoundingSphere(t, m_fEffectiveRange * 0.5f);

  float fScreenSpaceSize = CalculateScreenSpaceSize(bs, *msg.m_pView->GetCullingCamera());

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  if (CVarVisLightSize)
  {
    ezVec3 textPos;
    msg.m_pView->ComputeScreenSpacePos(t.m_vPosition, textPos);

    ezStringBuilder sb;
    sb.Format("{0}", fScreenSpaceSize);
    ezDebugRenderer::Draw2DText(msg.m_pView->GetHandle(), sb, ezVec2I32((int)textPos.x, (int)textPos.y), ezColor::Olive);
    ezDebugRenderer::DrawLineSphere(msg.m_pView->GetHandle(), bs, ezColor::Olive);
  }
#endif

  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;
  auto pRenderData = ezCreateRenderDataForThisFrame<ezSpotLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = t;
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_InnerSpotAngle = m_InnerSpotAngle;
  pRenderData->m_OuterSpotAngle = m_OuterSpotAngle;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_uiShadowDataOffset = m_bCastShadows ? ezShadowPool::AddSpotLight(this, fScreenSpaceSize) : ezInvalidIndex;

  ezRenderData::Caching::Enum caching = m_bCastShadows ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId, caching);
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
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_fRange;
  s >> m_InnerSpotAngle;
  s >> m_OuterSpotAngle;

  ezStringBuilder temp;
  s >> temp;
  SetProjectedTextureFile(temp);
}

ezBoundingSphere ezSpotLightComponent::CalculateBoundingSphere(const ezTransform& t, float fRange) const
{
  ezBoundingSphere res;
  ezAngle halfAngle = m_OuterSpotAngle / 2.0f;
  ezVec3 position = t.m_vPosition;
  ezVec3 forwardDir = t.m_qRotation * ezVec3(1.0f, 0.0f, 0.0f);

  if (halfAngle > ezAngle::Degree(45.0f))
  {
    res.m_vCenter = position + ezMath::Cos(halfAngle) * fRange * forwardDir;
    res.m_fRadius = ezMath::Sin(halfAngle) * fRange;
  }
  else
  {
    res.m_fRadius = fRange / (2.0f * ezMath::Cos(halfAngle));
    res.m_vCenter = position + forwardDir * res.m_fRadius;
  }

  return res;
}

//////////////////////////////////////////////////////////////////////////

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpotLightVisualizerAttribute, 1, ezRTTIDefaultAllocator<ezSpotLightVisualizerAttribute>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSpotLightVisualizerAttribute::ezSpotLightVisualizerAttribute()
    : ezVisualizerAttribute(nullptr)
{
}

ezSpotLightVisualizerAttribute::ezSpotLightVisualizerAttribute(const char* szAngleProperty, const char* szRangeProperty,
                                                               const char* szIntensityProperty, const char* szColorProperty)
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
      : ezGraphPatch("ezSpotLightComponent", 2)
  {
  }

  virtual void Patch(ezGraphPatchContext& context, ezAbstractObjectGraph* pGraph, ezAbstractObjectNode* pNode) const override
  {
    context.PatchBaseClass("ezLightComponent", 2, true);

    pNode->RenameProperty("Inner Spot Angle", "InnerSpotAngle");
    pNode->RenameProperty("Outer Spot Angle", "OuterSpotAngle");
  }
};

ezSpotLightComponentPatch_1_2 g_ezSpotLightComponentPatch_1_2;


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);
