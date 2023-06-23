#include <EditorPluginFmod/EditorPluginFmodPCH.h>

#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetProperties, 1, ezRTTIDefaultAllocator<ezSoundEventAssetProperties>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSoundEventAssetDocument::ezSoundEventAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezSoundEventAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

void ezSoundEventAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);
}

ezTransformStatus ezSoundEventAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
  const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  return ezStatus(EZ_SUCCESS);
}
