#include <RendererCore/PCH.h>
#include <RendererCore/Lights/SpotLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSpotLightRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_COMPONENT_TYPE(ezSpotLightComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant() ), new ezDefaultValueAttribute( 1.0f ) ),
    EZ_ACCESSOR_PROPERTY("Spot Angle", GetSpotAngle_Ui, SetSpotAngle_Ui)->AddAttributes(new ezClampValueAttribute(0.1f, 179.9f), new ezDefaultValueAttribute(30.0f)),
    EZ_ACCESSOR_PROPERTY("Projected Texture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture 2D")),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Graphics"),
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

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

  ezResourceLock<ezTextureResource> pTexture(m_hProjectedTexture);
  return pTexture->GetResourceID();
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
  ezBoundingSphere BoundingSphere(ezVec3::ZeroVector(), m_fRange);
  msg.m_ResultingLocalBounds.ExpandToInclude(BoundingSphere);
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


void ezSpotLightComponent::SetSpotAngle_Ui(float fSpotAngleInDegrees)
{
  m_SpotAngle = ezAngle::Degree( fSpotAngleInDegrees );
}

float ezSpotLightComponent::GetSpotAngle_Ui() const
{
  return m_SpotAngle.GetDegree();
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SpotLightComponent);

