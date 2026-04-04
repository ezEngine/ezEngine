#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/DiskLightComponent.h>
#include <RendererCore/Lights/Implementation/ShadowPool.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDiskLightRenderData, 1, ezRTTIDefaultAllocator<ezDiskLightRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezDiskLightComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new ezClampValueAttribute(0.01f, ezVariant()), new ezDefaultValueAttribute(0.5f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Range", GetRange, SetRange)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(5.0f), new ezSuffixAttribute(" m")),
    EZ_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("TwoSided", GetTwoSided, SetTwoSided)->AddAttributes(new ezDefaultValueAttribute(false)),
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

ezDiskLightComponent::ezDiskLightComponent() = default;
ezDiskLightComponent::~ezDiskLightComponent() = default;

ezResult ezDiskLightComponent::GetLocalBounds(ezBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, ezMsgUpdateLocalBounds& ref_msg)
{
  m_fEffectiveRange = m_fRange;

  const float fDiag = ezMath::Sqrt(m_fRadius * m_fRadius + m_fEffectiveRange * m_fEffectiveRange);
  ref_bounds = ezBoundingSphere::MakeFromCenterAndRadius(ezVec3::MakeZero(), fDiag);
  return EZ_SUCCESS;
}

void ezDiskLightComponent::SetRadius(float fRadius)
{
  m_fRadius = ezMath::Max(fRadius, 0.01f);
  TriggerLocalBoundsUpdate();
}

float ezDiskLightComponent::GetRadius() const
{
  return m_fRadius;
}

void ezDiskLightComponent::SetRange(float fRange)
{
  m_fRange = ezMath::Max(fRange, 0.0f);
  TriggerLocalBoundsUpdate();
}

float ezDiskLightComponent::GetRange() const
{
  return m_fRange;
}

float ezDiskLightComponent::GetEffectiveRange() const
{
  return m_fEffectiveRange;
}

void ezDiskLightComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = fFalloff;
}

float ezDiskLightComponent::GetFalloff() const
{
  return m_fFalloff;
}

void ezDiskLightComponent::SetTwoSided(bool bTwoSided)
{
  m_bTwoSided = bTwoSided;
  InvalidateCachedRenderData();
}

bool ezDiskLightComponent::GetTwoSided() const
{
  return m_bTwoSided;
}

void ezDiskLightComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow)
    return;

  if (m_fIntensity <= 0.0f || m_fEffectiveRange <= 0.0f)
    return;

  const ezTransform t = GetOwner()->GetGlobalTransform();
  const ezBoundingSphere bounds = ezBoundingSphere::MakeFromCenterAndRadius(t.m_vPosition, m_fEffectiveRange);
  const float fScreenSpaceSize = CalculateScreenSpaceSize(bounds, *msg.m_pView->GetCullingCamera());

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezDiskLightRenderData>(GetOwner());

  pRenderData->m_LightColor = GetEffectiveColor();
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fSpecularMultiplier = m_fSpecularMultiplier;

  pRenderData->m_qGlobalRotation = t.m_qRotation;
  pRenderData->m_fRadius = m_fRadius;
  pRenderData->m_fRange = m_fEffectiveRange;
  pRenderData->m_fFalloff = m_fFalloff;
  pRenderData->m_bTwoSided = m_bTwoSided;

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

  ezRenderData::Caching::Enum caching = m_bCastShadows ? ezRenderData::Caching::Never : ezRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, caching);
}

void ezDiskLightComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fRange;
  s << m_fFalloff;
  s << m_bTwoSided;
}

void ezDiskLightComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fRange;
  s >> m_fFalloff;
  s >> m_bTwoSided;
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DiskLightComponent);
