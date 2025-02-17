#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFogRenderData, 1, ezRTTIDefaultAllocator<ezFogRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezFogComponent, 2, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezDefaultValueAttribute(ezColorGammaUB(ezColor(0.2f, 0.2f, 0.3f)))),
    EZ_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("HeightFalloff", GetHeightFalloff, SetHeightFalloff)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f)),
    EZ_ACCESSOR_PROPERTY("ModulateWithSkyColor", GetModulateWithSkyColor, SetModulateWithSkyColor),
    EZ_ACCESSOR_PROPERTY("SkyDistance", GetSkyDistance, SetSkyDistance)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1000.0f)),
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
EZ_END_COMPONENT_TYPE
// clang-format on

ezFogComponent::ezFogComponent() = default;
ezFogComponent::~ezFogComponent() = default;

void ezFogComponent::Deinitialize()
{
  ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void ezFogComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezFogComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void ezFogComponent::SetColor(ezColor color)
{
  m_Color = color;

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

ezColor ezFogComponent::GetColor() const
{
  return m_Color;
}

void ezFogComponent::SetDensity(float fDensity)
{
  m_fDensity = ezMath::Max(fDensity, 0.0f);

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float ezFogComponent::GetDensity() const
{
  return m_fDensity;
}

void ezFogComponent::SetHeightFalloff(float fHeightFalloff)
{
  m_fHeightFalloff = ezMath::Max(fHeightFalloff, 0.0f);

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float ezFogComponent::GetHeightFalloff() const
{
  return m_fHeightFalloff;
}

void ezFogComponent::SetModulateWithSkyColor(bool bModulate)
{
  m_bModulateWithSkyColor = bModulate;

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

bool ezFogComponent::GetModulateWithSkyColor() const
{
  return m_bModulateWithSkyColor;
}

void ezFogComponent::SetSkyDistance(float fDistance)
{
  m_fSkyDistance = fDistance;

  if (IsActiveAndInitialized())
  {
    ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
  }
}

float ezFogComponent::GetSkyDistance() const
{
  return m_fSkyDistance;
}

void ezFogComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezFogComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezFogRenderData>(GetOwner());

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_Color = m_Color;
  pRenderData->m_fDensity = m_fDensity / 100.0f;
  pRenderData->m_fHeightFalloff = m_fHeightFalloff;
  pRenderData->m_fInvSkyDistance = m_bModulateWithSkyColor ? 1.0f / m_fSkyDistance : 0.0f;

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, ezRenderData::Caching::IfStatic);
}

void ezFogComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  ezStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_fDensity;
  s << m_fHeightFalloff;
  s << m_fSkyDistance;
  s << m_bModulateWithSkyColor;
}

void ezFogComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_fDensity;
  s >> m_fHeightFalloff;

  if (uiVersion >= 2)
  {
    s >> m_fSkyDistance;
    s >> m_bModulateWithSkyColor;
  }
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_FogComponent);
