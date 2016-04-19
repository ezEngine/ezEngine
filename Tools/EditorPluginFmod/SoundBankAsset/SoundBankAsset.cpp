#include <PCH.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAsset.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetObjects.h>
#include <EditorPluginFmod/SoundBankAsset/SoundBankAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <Foundation/IO/ChunkStream.h>


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSoundBankAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE


ezSoundBankAssetDocument::ezSoundBankAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezSoundBankAssetProperties>(szDocumentPath)
{
}

void ezSoundBankAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezSoundBankAssetProperties* pProp = GetProperties();


}

ezStatus ezSoundBankAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform)
{
  const ezSoundBankAssetProperties* pProp = GetProperties();

  return ezStatus(EZ_SUCCESS);
}


ezStatus ezSoundBankAssetDocument::InternalRetrieveAssetInfo(const char * szPlatform)
{


  return ezStatus(EZ_SUCCESS);
}

