#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/CustomDataAsset/CustomDataAssetProperties.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCustomDataAssetProperties, 1, ezRTTIDefaultAllocator<ezCustomDataAssetProperties>)
EZ_BEGIN_PROPERTIES
{
  EZ_MEMBER_PROPERTY("Type", m_pType)->AddFlags(ezPropertyFlags::PointerOwner),
}
EZ_END_PROPERTIES;
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
