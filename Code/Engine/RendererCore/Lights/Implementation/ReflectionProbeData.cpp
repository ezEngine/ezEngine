#include <RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>

namespace
{
  static ezVariantArray GetDefaultTags()
  {
    ezVariantArray value(ezStaticAllocatorWrapper::GetAllocator());
    value.PushBack(ezStringView("SkyLight"));
    return value;
  }
}

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezReflectionProbeData, ezNoBase, 1, ezRTTINoAllocator)
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

  return EZ_SUCCESS;
}

ezResult ezReflectionProbeData::Deserialize(ezStreamReader& stream)
{
  m_IncludeTags.Load(stream, ezTagRegistry::GetGlobalRegistry());
  m_ExcludeTags.Load(stream, ezTagRegistry::GetGlobalRegistry());
  stream >> m_Mode;
  stream >> m_bShowDebugInfo;
  stream >> m_fIntensity;
  stream >> m_fSaturation;

  return EZ_SUCCESS;
}
