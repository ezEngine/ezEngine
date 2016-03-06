#include <RendererCore/PCH.h>
#include <RendererCore/Lights/PointLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPointLightRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_COMPONENT_TYPE(ezPointLightComponent, 1);
  EZ_BEGIN_PROPERTIES
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant() ), new ezDefaultValueAttribute( 1.0f ) ),
    EZ_ACCESSOR_PROPERTY("Projected Texture", GetProjectedTextureFile, SetProjectedTextureFile)->AddAttributes(new ezAssetBrowserAttribute("Texture Cube")),
  EZ_END_PROPERTIES
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Graphics"),
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

ezPointLightComponent::ezPointLightComponent()
  : m_fRange(1.0f)
{
}

void ezPointLightComponent::SetRange(float fRange)
{
  m_fRange = fRange;

  if (IsActive())
    GetOwner()->UpdateLocalBounds();
}

float ezPointLightComponent::GetRange() const
{
  return m_fRange;
}

void ezPointLightComponent::SetProjectedTexture(const ezTextureResourceHandle& hProjectedTexture)
{
  m_hProjectedTexture = hProjectedTexture;
}

const ezTextureResourceHandle& ezPointLightComponent::GetProjectedTexture() const
{
  return m_hProjectedTexture;
}

void ezPointLightComponent::SetProjectedTextureFile(const char* szFile)
{
  ezTextureResourceHandle hProjectedTexture;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hProjectedTexture = ezResourceManager::LoadResource<ezTextureResource>(szFile);
  }

  SetProjectedTexture(hProjectedTexture);
}

const char* ezPointLightComponent::GetProjectedTextureFile() const
{
  if (!m_hProjectedTexture.IsValid())
    return "";

  ezResourceLock<ezTextureResource> pTexture(m_hProjectedTexture);
  return pTexture->GetResourceID();
}

void ezPointLightComponent::OnAfterAttachedToObject()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezPointLightComponent::OnBeforeDetachedFromObject()
{
  if (IsActive())
  {
    // temporary set to inactive so we don't receive the msg
    SetActive(false);
    GetOwner()->UpdateLocalBounds();
    SetActive(true);
  }
}

void ezPointLightComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  ezBoundingSphere BoundingSphere(ezVec3::ZeroVector(), m_fRange);
  msg.m_ResultingLocalBounds.ExpandToInclude(BoundingSphere);
}

void ezPointLightComponent::OnExtractRenderData( ezExtractRenderDataMessage& msg ) const
{
  ezUInt32 uiBatchId = m_bCastShadows ? 0 : 1;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezPointLightRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fRange = m_fRange;
  pRenderData->m_hProjectedTexture = m_hProjectedTexture;
  pRenderData->m_bCastShadows = m_bCastShadows;

  msg.m_pExtractedRenderData->AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light);
}

void ezPointLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  ezLightComponent::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_fRange;
  s << GetProjectedTextureFile();
}

void ezPointLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  ezLightComponent::DeserializeComponent(stream);

  ezStreamReader& s = stream.GetStream();

  s >> m_fRange;

  ezStringBuilder temp;
  s >> temp;
  SetProjectedTextureFile(temp);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_PointLightComponent);

