#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetObjects.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezImageDataAssetProperties, 1, ezRTTIDefaultAllocator<ezImageDataAssetProperties>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Input", m_sInputFile)->AddAttributes(new ezFileBrowserAttribute("Select Image", "*.dds;*.tga;*.png;*.jpg;*.jpeg;*.hdr"))
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
