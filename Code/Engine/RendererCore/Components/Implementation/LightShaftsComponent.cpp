#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/LightShaftsComponent.h>
#include <RendererCore/Pipeline/RenderDataManager.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLightShaftsRenderData, 1, ezRTTIDefaultAllocator<ezLightShaftsRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezLightShaftsComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("BrightnessThreshold", GetBrightnessThreshold, SetBrightnessThreshold)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(0.0f)),
    EZ_ACCESSOR_PROPERTY("MaxBrightness", GetMaxBrightness, SetMaxBrightness)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f)),
    EZ_ACCESSOR_PROPERTY("DiskMaskRadius", GetDiskMaskRadius, SetDiskMaskRadius)->AddAttributes(new ezClampValueAttribute(0.0f, 2.0f), new ezDefaultValueAttribute(0.1f)),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnMsgExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Effects"),
  }
  EZ_END_ATTRIBUTES;  
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezLightShaftsComponent::ezLightShaftsComponent() = default;
ezLightShaftsComponent::~ezLightShaftsComponent() = default;

void ezLightShaftsComponent::Deinitialize()
{
  ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void ezLightShaftsComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezLightShaftsComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezLightShaftsComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  ezStreamWriter& s = inout_stream.GetStream();

  s << m_fIntensity;
  s << m_fMaxBrightness;
  s << m_fBrightnessThreshold;
  s << m_fDiskMaskRadius;
}

void ezLightShaftsComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = inout_stream.GetStream();

  s >> m_fIntensity;
  s >> m_fMaxBrightness;
  s >> m_fBrightnessThreshold;
  s >> m_fDiskMaskRadius;
}

void ezLightShaftsComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = ezMath::Max(fIntensity, 0.0f);

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void ezLightShaftsComponent::SetBrightnessThreshold(float fBrightnessThreshold)
{
  m_fBrightnessThreshold = ezMath::Max(fBrightnessThreshold, 0.0f);

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void ezLightShaftsComponent::SetMaxBrightness(float fMaxBrightness)
{
  m_fMaxBrightness = ezMath::Max(fMaxBrightness, 0.0f);

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void ezLightShaftsComponent::SetDiskMaskRadius(float fDiskMaskRadius)
{
  m_fDiskMaskRadius = ezMath::Clamp(fDiskMaskRadius, 0.0f, 2.0f);

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

void ezLightShaftsComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezLightShaftsComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't render in shadow and reflection views
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Reflection)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  auto pRenderData = msg.m_pRenderDataManager->CreateRenderDataForThisFrame<ezLightShaftsRenderData>(GetOwner());

  pRenderData->m_vDirection = GetOwner()->GetGlobalRotation() * ezVec3(-1, 0, 0);
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_fMaxBrightness = m_fMaxBrightness;
  pRenderData->m_fBrightnessThreshold = m_fBrightnessThreshold;
  pRenderData->m_fDiskMaskRadius = m_fDiskMaskRadius;

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, ezRenderData::Caching::IfStatic);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_LightShaftsComponent);
