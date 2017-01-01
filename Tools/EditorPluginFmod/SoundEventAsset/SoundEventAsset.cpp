#include <PCH.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAsset.h>
#include <EditorPluginFmod/SoundEventAsset/SoundEventAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/OSFile.h>

#include <fmod_studio.hpp>
#define EZ_FMOD_ASSERT(res) EZ_VERIFY((res) == FMOD_OK, "Fmod failed with error code {0}", res)


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundEventAssetProperties, 1, ezRTTIDefaultAllocator<ezSoundEventAssetProperties>)
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSoundEventAssetDocument::ezSoundEventAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezSoundEventAssetProperties>(szDocumentPath)
{
}

void ezSoundEventAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  ezAssetDocument::UpdateAssetDocumentInfo(pInfo);

  const ezSoundEventAssetProperties* pProp = GetProperties();

}

ezStatus ezSoundEventAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  const ezSoundEventAssetProperties* pProp = GetProperties();

  return ezStatus(EZ_SUCCESS);
}

