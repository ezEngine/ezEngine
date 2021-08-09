#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

namespace
{
  static ezVariantArray GetDefaultTags()
  {
    ezVariantArray value(ezStaticAllocatorWrapper::GetAllocator());
    value.PushBack(ezStringView("SkyLight"));
    return value;
  }
} // namespace

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_ENUM(ezReflectionProbeMode, 1)
  EZ_BITFLAGS_CONSTANTS(ezReflectionProbeMode::Static, ezReflectionProbeMode::Dynamic)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_STATIC_REFLECTED_TYPE(ezReflectionProbeData, ezNoBase, 2, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_MEMBER_PROPERTY("IncludeTags", m_IncludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default"), new ezDefaultValueAttribute(GetDefaultTags())),
    EZ_SET_MEMBER_PROPERTY("ExcludeTags", m_ExcludeTags)->AddAttributes(new ezTagSetWidgetAttribute("Default")),
    EZ_MEMBER_PROPERTY("ShowDebugInfo", m_bShowDebugInfo),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezResult ezReflectionProbeData::Serialize(ezStreamWriter& stream) const
{
  m_IncludeTags.Save(stream);
  m_ExcludeTags.Save(stream);
  stream << m_Mode;
  stream << m_bShowDebugInfo;
  stream << m_fIntensity;
  stream << m_fSaturation;
  stream << m_hCubeMap;
  return EZ_SUCCESS;
}

ezResult ezReflectionProbeData::Deserialize(ezStreamReader& stream, const ezUInt32 uiVersion)
{
  m_IncludeTags.Load(stream, ezTagRegistry::GetGlobalRegistry());
  m_ExcludeTags.Load(stream, ezTagRegistry::GetGlobalRegistry());
  stream >> m_Mode;
  stream >> m_bShowDebugInfo;
  stream >> m_fIntensity;
  stream >> m_fSaturation;
  if (uiVersion >= 2)
  {
    stream >> m_hCubeMap;
  }
  return EZ_SUCCESS;
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeData);
