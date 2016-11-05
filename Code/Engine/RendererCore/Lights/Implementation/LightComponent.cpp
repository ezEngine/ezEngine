#include <RendererCore/PCH.h>
#include <RendererCore/Lights/LightComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLightRenderData, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezLightComponent, 1)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Light Color", m_LightColor),
    EZ_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(10.0f)),
    //EZ_MEMBER_PROPERTY("Cast Shadows", m_bCastShadows),
  }
  EZ_END_PROPERTIES
    EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Lighting"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_ABSTRACT_COMPONENT_TYPE

ezLightComponent::ezLightComponent()
  : m_LightColor(ezColor::White)
  , m_fIntensity(10.0f)
  , m_bCastShadows(false)
{
}


void ezLightComponent::SetLightColor(ezColorGammaUB LightColor)
{
  m_LightColor = LightColor;
}

ezColorGammaUB ezLightComponent::GetLightColor() const
{
  return m_LightColor;
}

void ezLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;
}

float ezLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void ezLightComponent::SetCastShadows(bool bCastShadows)
{
  m_bCastShadows = bCastShadows;
}

bool ezLightComponent::GetCastShadows()
{
  return m_bCastShadows;
}


void ezLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  ezStreamWriter& s = stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_bCastShadows;
}

void ezLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  ezStreamReader& s = stream.GetStream();

  s >> m_LightColor;
  s >> m_fIntensity;
  s >> m_bCastShadows;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);

