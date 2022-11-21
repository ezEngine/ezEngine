#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingInterface.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezBakingSettings, ezNoBase, 1, ezRTTIDefaultAllocator<ezBakingSettings>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ProbeSpacing", m_vProbeSpacing)->AddAttributes(new ezDefaultValueAttribute(ezVec3(4)), new ezClampValueAttribute(ezVec3(0.1f), ezVariant())),
    EZ_MEMBER_PROPERTY("NumSamplesPerProbe", m_uiNumSamplesPerProbe)->AddAttributes(new ezDefaultValueAttribute(128), new ezClampValueAttribute(32, 1024)),
    EZ_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)->AddAttributes(new ezDefaultValueAttribute(1000), new ezClampValueAttribute(1, ezVariant())),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

static ezTypeVersion s_BakingSettingsVersion = 1;
ezResult ezBakingSettings::Serialize(ezStreamWriter& stream) const
{
  stream.WriteVersion(s_BakingSettingsVersion);

  stream << m_vProbeSpacing;
  stream << m_uiNumSamplesPerProbe;
  stream << m_fMaxRayDistance;

  return EZ_SUCCESS;
}

ezResult ezBakingSettings::Deserialize(ezStreamReader& stream)
{
  const ezTypeVersion version = stream.ReadVersion(s_BakingSettingsVersion);

  stream >> m_vProbeSpacing;
  stream >> m_uiNumSamplesPerProbe;
  stream >> m_fMaxRayDistance;

  return EZ_SUCCESS;
}
