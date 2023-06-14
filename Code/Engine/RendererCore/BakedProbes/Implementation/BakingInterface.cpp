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
ezResult ezBakingSettings::Serialize(ezStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_BakingSettingsVersion);

  inout_stream << m_vProbeSpacing;
  inout_stream << m_uiNumSamplesPerProbe;
  inout_stream << m_fMaxRayDistance;

  return EZ_SUCCESS;
}

ezResult ezBakingSettings::Deserialize(ezStreamReader& inout_stream)
{
  const ezTypeVersion version = inout_stream.ReadVersion(s_BakingSettingsVersion);
  EZ_IGNORE_UNUSED(version);

  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_uiNumSamplesPerProbe;
  inout_stream >> m_fMaxRayDistance;

  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakingInterface);
