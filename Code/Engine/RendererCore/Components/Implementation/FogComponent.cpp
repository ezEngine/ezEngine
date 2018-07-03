#include <PCH.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezFogRenderData, 1, ezRTTIDefaultAllocator<ezFogRenderData>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_COMPONENT_TYPE(ezFogComponent, 1, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new ezDefaultValueAttribute(ezColorGammaUB(ezColor(0.2f, 0.2f, 0.3f)))),
    EZ_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("HeightFalloff", GetHeightFalloff, SetHeightFalloff)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f))
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE

ezFogComponent::ezFogComponent()
  : m_Color(ezColor(0.2f, 0.2f, 0.3f))
  , m_fDensity(1.0f)
  , m_fHeightFalloff(10.0f)
{
}

ezFogComponent::~ezFogComponent()
{

}

void ezFogComponent::Deinitialize()
{
  ezRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());
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
  SetModified(EZ_BIT(1));
}

ezColor ezFogComponent::GetColor() const
{
  return m_Color;
}

void ezFogComponent::SetDensity(float fDensity)
{
  m_fDensity = ezMath::Max(fDensity, 0.0f);
  SetModified(EZ_BIT(2));
}

float ezFogComponent::GetDensity() const
{
  return m_fDensity;
}

void ezFogComponent::SetHeightFalloff(float fHeightFalloff)
{
  m_fHeightFalloff = ezMath::Max(fHeightFalloff, 0.0f);
  SetModified(EZ_BIT(3));
}

float ezFogComponent::GetHeightFalloff() const
{
  return m_fHeightFalloff;
}


void ezFogComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible();
}

void ezFogComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != ezInvalidRenderDataCategory)
    return;

  ezUInt32 uiBatchId = 0;

  auto pRenderData = ezCreateRenderDataForThisFrame<ezFogRenderData>(GetOwner(), uiBatchId);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_Color = m_Color;
  pRenderData->m_fDensity = m_fDensity / 100.0f;
  pRenderData->m_fHeightFalloff = m_fHeightFalloff;

  msg.AddRenderData(pRenderData, ezDefaultRenderDataCategories::Light, uiBatchId, ezRenderData::Caching::IfStatic);
}

void ezFogComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  s << m_Color;
  s << m_fDensity;
  s << m_fHeightFalloff;
}

void ezFogComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  //const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  s >> m_Color;
  s >> m_fDensity;
  s >> m_fHeightFalloff;
}




EZ_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_FogComponent);

