#include <PCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetManager.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetProperties, 1, ezRTTIDefaultAllocator<ezSoundEventAssetProperties>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSoundEventAssetDocument::ezSoundEventAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezSoundEventAssetProperties>(szDocumentPath)
{
}

void ezSoundEventAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  const ezSoundEventAssetProperties* pProp = GetProperties();
}

ezStatus ezSoundEventAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezAssetProfile* pAssetProfile,
                                                           const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  const ezSoundEventAssetProperties* pProp = GetProperties();

  return ezStatus(EZ_SUCCESS);
}
