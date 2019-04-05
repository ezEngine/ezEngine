#include <RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSkyLightComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_MEMBER_PROPERTY("ReflectionData", m_ReflectionProbeData)
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgUpdateLocalBounds, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezMsgExtractRenderData, OnExtractRenderData),
  }
  EZ_END_MESSAGEHANDLERS;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Rendering/Lighting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

static ezUInt32 s_uiSkyLightPriority = 10;

ezSkyLightComponent::ezSkyLightComponent()
{
}

ezSkyLightComponent::~ezSkyLightComponent() = default;

void ezSkyLightComponent::OnActivated()
{
  ezReflectionPool::RegisterReflectionProbe(m_ReflectionProbeData, GetWorld(), s_uiSkyLightPriority);

  GetOwner()->UpdateLocalBounds();
}

void ezSkyLightComponent::OnDeactivated()
{
  ezReflectionPool::DeregisterReflectionProbe(m_ReflectionProbeData, GetWorld());

  GetOwner()->UpdateLocalBounds();
}

void ezSkyLightComponent::SetIntensity(float fIntensity)
{
  m_ReflectionProbeData.m_fIntensity = fIntensity;
}

float ezSkyLightComponent::GetIntensity() const
{
  return m_ReflectionProbeData.m_fIntensity;
}

void ezSkyLightComponent::SetSaturation(float fSaturation)
{
  m_ReflectionProbeData.m_fSaturation = fSaturation;
}

float ezSkyLightComponent::GetSaturation() const
{
  return m_ReflectionProbeData.m_fSaturation;
}

void ezSkyLightComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible();
}

void ezSkyLightComponent::OnExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow ||
      msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Reflection)
    return;

  if (m_ReflectionProbeData.m_fIntensity <= 0.0f)
    return;

  ezReflectionPool::ExtractReflectionProbe(msg, m_ReflectionProbeData, this, s_uiSkyLightPriority);
}

void ezSkyLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  m_ReflectionProbeData.Serialize(s);
}

void ezSkyLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  m_ReflectionProbeData.Deserialize(s);
}
