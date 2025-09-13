#include <EditorPluginRmlUi/EditorPluginRmlUiPCH.h>

#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiAssetProperties, 1, ezRTTIDefaultAllocator<ezRmlUiAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RmlFile", m_sRmlFile)->AddAttributes(new ezFileBrowserAttribute("Select Rml file", "*.rml", {}, "RmlUI")),
    EZ_ENUM_MEMBER_PROPERTY("ScaleMode", ezRmlUiScaleMode, m_ScaleMode),
    EZ_MEMBER_PROPERTY("ReferenceResolution", m_ReferenceResolution)->AddAttributes(new ezDefaultValueAttribute(ezVec2U32(1920, 1080))),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRmlUiAssetProperties::ezRmlUiAssetProperties() = default;
ezRmlUiAssetProperties::~ezRmlUiAssetProperties() = default;
