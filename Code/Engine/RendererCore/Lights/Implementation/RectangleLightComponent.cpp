#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Decals/Implementation/DecalManager.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Lights/RectangleLightComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRectangleLightRenderData, 1, ezRTTIDefaultAllocator<ezRectangleLightRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezRectangleLightComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new ezClampValueAttribute(0.01f, ezVariant()), new ezDefaultValueAttribute(1.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new ezClampValueAttribute(0.01f, ezVariant()), new ezDefaultValueAttribute(1.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(5.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("TwoSided", GetTwoSided, SetTwoSided)->AddAttributes(new ezDefaultValueAttribute(false)),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Cookie", GetCookie, SetCookie)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    EZ_RESOURCE_ACCESSOR_PROPERTY("Material", GetMaterial, SetMaterial)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Material", "Decal")),
    EZ_ACCESSOR_PROPERTY("MaterialResolution", GetMaterialResolution, SetMaterialResolution)->AddAttributes(new ezClampValueAttribute(16, 1024), new ezDefaultValueAttribute(512)),
    EZ_ACCESSOR_PROPERTY("MaterialUpdateInterval", GetMaterialUpdateInterval, SetMaterialUpdateInterval)->AddAttributes(new ezClampValueAttribute(0.0, 10.0), new ezDefaultValueAttribute(0.0f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Lighting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezRectangleLightComponent::ezRectangleLightComponent() = default;
ezRectangleLightComponent::~ezRectangleLightComponent() = default;

void ezRectangleLightComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateCookie();
}

void ezRectangleLightComponent::OnDeactivated()
{
  DeleteCookie();

  SUPER::OnDeactivated();
}

ezResult ezRectangleLightComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = m_fRange;

  const float fHalfWidth = m_fWidth * 0.5f;
  const float fHalfHeight = m_fHeight * 0.5f;
  const float fDiag = ezMath::Sqrt(fHalfWidth * fHalfWidth + fHalfHeight * fHalfHeight + m_fEffectiveRange * m_fEffectiveRange);

  ref_bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), fDiag);
  return EZ_SUCCESS;
}

void ezRectangleLightComponent::SetWidth(float fWidth)
{
  m_fWidth = ezMath::Max(fWidth, 0.01f);
  TriggerLocalBoundsUpdate();
}

float ezRectangleLightComponent::GetWidth() const
{
  return m_fWidth;
}

void ezRectangleLightComponent::SetHeight(float fHeight)
{
  m_fHeight = ezMath::Max(fHeight, 0.01f);
  TriggerLocalBoundsUpdate();
}

float ezRectangleLightComponent::GetHeight() const
{
  return m_fHeight;
}

void ezRectangleLightComponent::SetRange(float fRange)
{
  m_fRange = ezMath::Max(fRange, 0.0f);
  TriggerLocalBoundsUpdate();
}

float ezRectangleLightComponent::GetRange() const
{
  return m_fRange;
}

float ezRectangleLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void ezRectangleLightComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = fFalloff;
}

float ezRectangleLightComponent::GetFalloff() const
{
  return m_fFalloff;
}

void ezRectangleLightComponent::SetTwoSided(bool bTwoSided)
{
  m_bTwoSided = bTwoSided;
  InvalidateCachedRenderData();
}

bool ezRectangleLightComponent::GetTwoSided() const
{
  return m_bTwoSided;
}

void ezRectangleLightComponent::SetCookie(const ezTexture2DResourceHandle& hCookie)
{
  if (m_hCookie != hCookie)
  {
    m_hCookie = hCookie;

    UpdateCookie();
    InvalidateCachedRenderData();
  }
}

void ezRectangleLightComponent::SetMaterial(const ezMaterialResourceHandle& hMaterial)
{
  if (m_hMaterial != hMaterial)
  {
    m_hMaterial = hMaterial;

    UpdateCookie();
    InvalidateCachedRenderData();
  }
}

void ezRectangleLightComponent::SetMaterialResolution(ezUInt32 uiResolution)
{
  m_uiMaterialResolution = ezMath::Clamp(uiResolution, 16u, 1024u);

  UpdateCookie();
}

void ezRectangleLightComponent::SetMaterialUpdateInterval(ezTime updateInterval)
{
  m_MaterialUpdateInterval = ezMath::Clamp(updateInterval.AsFloatInSeconds(), 0.0f, 10.0f);

  UpdateCookie();
}

void ezRectangleLightComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  const ezTransform t = GetOwner()->GetGlobalTransform();
  const ezBoundingSphere bounds = ezBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, m_fEffectiveRange);
  const float fScreenSpaceSize = CalculateScreenSpaceSize(bounds, *msg.m_pView->GetCullingCamera());

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezRectangleLightRenderData>(GetOwner());

  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;

  pRenderData->m_qGlobalRotation = t.m_qRotation;
  pRenderData->m_fWidth = m_fWidth;
  pRenderData->m_fHeight = m_fHeight;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_fFalloff = m_fFalloff;
  pRenderData->m_bTwoSided = m_bTwoSided;
  pRenderData->m_CookieId = m_CookieId;

  if (m_CookieId.IsInvalidated() == false)
  {
    ezDecalManager::MarkRuntimeDecalAsUsed(m_CookieId, fScreenSpaceSize, msg.m_pView);
  }

  if (m_bCastShadows)
  {
    float fShadowScreenSize = 0.0f;
    const float fShadowFadeOut = CalculateShadowFadeOut(bounds, 0.0f, *msg.m_pView->GetCullingCamera(), fShadowScreenSize);
    if (fShadowFadeOut > 0.0f)
    {
      pRenderData->FillShadowDataOffsetAndFadeOut(ezShadowPool::AddAreaLight(this, fScreenSpaceSize, msg.m_pView, m_fEffectiveRange), fShadowFadeOut);
    }
    else
    {
      pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
    }
  }
  else
  {
    pRenderData->m_uiShadowDataOffsetAndFadeOut = 0;
  }

  pRenderData->FillBatchIdAndSortingKey(fScreenSpaceSize);

  ezRenderData::Caching::Enum caching = (m_bCastShadows || m_CookieId.IsInvalidated() == false) ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, caching);
}

void ezRectangleLightComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fWidth;
  s << m_fHeight;
  s << m_fRange;
  s << m_fFalloff;
  s << m_bTwoSided;
  s << m_uiMaterialResolution;
  s << m_MaterialUpdateInterval;
  s << m_hMaterial;
  s << m_hCookie;
}

void ezRectangleLightComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fWidth;
  s >> m_fHeight;
  s >> m_fRange;
  s >> m_fFalloff;
  s >> m_bTwoSided;
  s >> m_uiMaterialResolution;
  s >> m_MaterialUpdateInterval;
  s >> m_hMaterial;
  s >> m_hCookie;
}

void ezRectangleLightComponent::UpdateCookie()
{
  if (!IsActiveAndInitialized())
    return;

  DeleteCookie();

  if (m_hMaterial.IsValid())
  {
    m_CookieId = ezDecalManager::GetOrCreateRuntimeDecal(m_hMaterial, m_uiMaterialResolution, ezTime::MakeFromSeconds(m_MaterialUpdateInterval));
  }
  else if (m_hCookie.IsValid())
  {
    m_CookieId = ezDecalManager::GetOrCreateRuntimeDecal(m_hCookie);
  }
}

void ezRectangleLightComponent::DeleteCookie()
{
  ezDecalManager::DeleteRuntimeDecal(m_CookieId);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_RectangleLightComponent);
