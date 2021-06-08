#include <RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSkyLightComponent, 2, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ACCESSOR_PROPERTY("Saturation", GetSaturation, SetSaturation)->AddAttributes(new ezClampValueAttribute(0.0f, ezVariant()), new ezDefaultValueAttribute(1.0f)),
    EZ_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", ezReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new ezDefaultValueAttribute(ezReflectionProbeMode::Dynamic)),
    EZ_ACCESSOR_PROPERTY("CubeMap", GetCubeMapFile, SetCubeMapFile)->AddAttributes(new ezAssetBrowserAttribute("Texture Cube")),
    EZ_MEMBER_PROPERTY("ReflectionData", m_ReflectionProbeData)
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
    new ezCategoryAttribute("Rendering/Lighting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

static float s_fSkyLightPriority = 10.0f;

ezSkyLightComponent::ezSkyLightComponent() = default;
ezSkyLightComponent::~ezSkyLightComponent() = default;

void ezSkyLightComponent::OnActivated()
{
  ezReflectionPool::RegisterReflectionProbe(m_ReflectionProbeData, GetWorld(), s_fSkyLightPriority);

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
  m_bStatesDirty = true;
}

float ezSkyLightComponent::GetIntensity() const
{
  return m_ReflectionProbeData.m_fIntensity;
}

void ezSkyLightComponent::SetSaturation(float fSaturation)
{
  m_ReflectionProbeData.m_fSaturation = fSaturation;
  m_bStatesDirty = true;
}

float ezSkyLightComponent::GetSaturation() const
{
  return m_ReflectionProbeData.m_fSaturation;
}

void ezSkyLightComponent::SetReflectionProbeMode(ezEnum<ezReflectionProbeMode> mode)
{
  m_ReflectionProbeData.m_Mode = mode;
  m_bStatesDirty = true;
}

ezEnum<ezReflectionProbeMode> ezSkyLightComponent::GetReflectionProbeMode() const
{
  return m_ReflectionProbeData.m_Mode;
}

void ezSkyLightComponent::SetCubeMapFile(const char* szFile)
{
  ezTextureCubeResourceHandle hCubeMap;
  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hCubeMap = ezResourceManager::LoadResource<ezTextureCubeResource>(szFile);
  }
  m_ReflectionProbeData.m_hCubeMap = hCubeMap;
  m_bStatesDirty = true;
}

const char* ezSkyLightComponent::GetCubeMapFile() const
{
  return m_ReflectionProbeData.m_hCubeMap.IsValid() ? m_ReflectionProbeData.m_hCubeMap.GetResourceID().GetData() : "";
}

void ezSkyLightComponent::OnUpdateLocalBounds(ezMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? ezDefaultSpatialDataCategories::RenderDynamic : ezDefaultSpatialDataCategories::RenderStatic);
}

void ezSkyLightComponent::OnMsgExtractRenderData(ezMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == ezCameraUsageHint::Reflection)
    return;

  if (m_ReflectionProbeData.m_fIntensity <= 0.0f || m_ReflectionProbeData.m_Mode == ezReflectionProbeMode::Static && !m_ReflectionProbeData.m_hCubeMap.IsValid())
    return;

  ezReflectionPool::ExtractReflectionProbe(msg, m_ReflectionProbeData, this, m_bStatesDirty, s_fSkyLightPriority);
}

void ezSkyLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();

  m_ReflectionProbeData.Serialize(s).IgnoreResult();
}

void ezSkyLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  ezStreamReader& s = stream.GetStream();

  ezUInt32 uiReflectionProbeDataVersion = 1;
  if (uiVersion >= 2)
  {
    uiReflectionProbeDataVersion = 2;
  }
  m_ReflectionProbeData.Deserialize(s, uiReflectionProbeDataVersion).IgnoreResult();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SkyLightComponent);
