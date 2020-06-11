#include <EditorPluginRmlUiPCH.h>

#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiAssetProperties, 1, ezRTTIDefaultAllocator<ezRmlUiAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("RmlFile", m_sRmlFile)->AddAttributes(new ezFileBrowserAttribute("Select Rml file", "*.rml")),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezRmlUiAssetProperties::ezRmlUiAssetProperties() = default;
ezRmlUiAssetProperties::~ezRmlUiAssetProperties() = default;
