#include <RendererCore/PCH.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpotLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezSpotLightComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f)),
    EZ_ACCESSOR_PROPERTY("Inner Spot Angle", GetInnerSpotAngle, SetInnerSpotAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(179.0f)), new ezDefaultValueAttribute(ezAngle::Degree(15.0f))),
    EZ_ACCESSOR_PROPERTY("Outer Spot Angle", GetOuterSpotAngle, SetOuterSpotAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(0.0f), ezAngle::Degree(179.0f)), new ezDefaultValueAttribute(ezAngle::Degree(30.0f))),
    //EZ_ACCESSOR_PROPERTY("Projected Texture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveX, "Outer Spot Angle", 1.0f, "Range", "Light Color"),
    new ezConeManipulatorAttribute("Outer Spot Angle", "Range"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezSpotLightComponent::ezSpotLightComponent()
  : m_fRange(10.0f)
  , m_InnerSpotAngle(ezAngle::Degree(15.0f))
  , m_OuterSpotAngle(ezAngle::Degree(30.0f))
{
}

ezSpotLightComponent::~ezSpotLightComponent()
{

}

ezResult ezSpotLightComponent::GetLocalBounds(ezBoundingBoxSphere& bounds)
{
  const float t = ezMath::Tan(m_OuterSpotAngle * 0.5f);
  const float p = ezMath::Min(t * m_fRange, m_fRange);

  ezBoundingBox box;
  box.m_vMin = ezVec3(0.0f, -p, -p);
  box.m_vMax = ezVec3(m_fRange, p, p);

  bounds = box;
  return EZ_SUCCESS;
}

void ezSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
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

  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

ezAngle ezSpotLightComponent::GetOuterSpotAngle() const
{
  return m_OuterSpotAngle;
}

void ezSpotLightComponent::SetProjectedTexture(const ezTextureResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;
}

const ezTextureResourceHandle& ezSpotLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void ezSpotLightComponent::SetProjectedTextureFile(const char* szFile)
{
  ezTextureResourceHandle hProjectedTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = ezResourceManager::LoadResource<ezTextureResource>(szFile);
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
  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezSpotLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fRange;
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


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);

