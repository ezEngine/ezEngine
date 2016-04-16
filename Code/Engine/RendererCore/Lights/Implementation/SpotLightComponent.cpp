#include <RendererCore/PCH.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpotLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_COMPONENT_TYPE(ezSpotLightComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("Spot Angle", m_SpotAngle)->AddAttributes(new ezClampValueAttribute(ezAngle::Degree(1.0f), ezAngle::Degree(179.0f)), new ezDefaultValueAttribute(ezAngle::Degree(30.0f))),
    EZ_ACCESSOR_PROPERTY("Projected Texture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
    EZ_BEGIN_ATTRIBUTES
  {
    new ezConeVisualizerAttribute(ezBasisAxis::PositiveX, "Spot Angle", 1.0f, "Range", "Light Color"),
    new ezConeManipulatorAttribute("Spot Angle", "Range"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

ezSpotLightComponent::ezSpotLightComponent()
  : m_fRange(1.0f)
  , m_SpotAngle(ezAngle::Degree(30.0f))
{
}

void ezSpotLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  if (IsActive())
    GetOwner()->UpdateLocalBounds();
}

float ezSpotLightComponent::GetRange() const
{
  return m_fRange;
}

void ezSpotLightComponent::SetSpotAngle( ezAngle SpotAngle )
{
  m_SpotAngle = SpotAngle;
}

ezAngle ezSpotLightComponent::GetSpotAngle() const
{
  return m_SpotAngle;
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

void ezSpotLightComponent::OnAfterAttachedToObject()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezSpotLightComponent::OnBeforeDetachedFromObject()
{
  if (IsActive())
  {
    // temporary set to inactive so we don't receive the msg
    SetActive(false);
    GetOwner()->UpdateLocalBounds();
    SetActive(true);
  }
}

void ezSpotLightComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  const float t = ezMath::Tan(m_SpotAngle * 0.5f);

  const float p = t * m_fRange;

  ezBoundingBox box;
  box.m_vMin = ezVec3(0.0f, -p, -p);
  box.m_vMax = ezVec3(m_fRange, p, p);

  msg.m_ResultingLocalBounds.ExpandToInclude(box);
}

void ezSpotLightComponent::OnExtractRenderData( ezExtractRenderDataMessage& msg ) const
{
  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezSpotLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fRange;
  pRenderData->m_SpotAngle = m_SpotAngle;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_bCastShadows = m_bCastShadows;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId);
}

void ezSpotLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fRange;
  s << m_SpotAngle;
  s << GetProjectedTextureFile();
}

void ezSpotLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());


  ezStreamReader& s = stream.GetStream();

  s >> m_fRange;
  s >> m_SpotAngle;

  ezStringBuilder temp;
  s >> temp;
  SetProjectedTextureFile(temp);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);

