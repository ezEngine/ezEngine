#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/Util/AssetUtils.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezMaterialResourceSlot, ezNoBase, 1, ezRTTIDefaultAllocator<ezMaterialResourceSlot>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Label", m_sLabel)->AddAttributes(new ezReadOnlyAttribute()),
    EZ_MEMBER_PROPERTY("Resource", m_sResource)->AddAttributes(new ezAssetBrowserAttribute("Material")),
    EZ_MEMBER_PROPERTY("Highlight", m_bHighlight)->AddAttributes(new ezTemporaryAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on
