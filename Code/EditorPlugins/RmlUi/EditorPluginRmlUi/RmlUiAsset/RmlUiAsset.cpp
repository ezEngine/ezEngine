#include <EditorPluginRmlUiPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Utilities/Progress.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezRmlUiAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezRmlUiAssetDocument::ezRmlUiAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezRmlUiAssetProperties>(szDocumentPath, ezAssetDocEngineConnection::Simple)
{
}

//////////////////////////////////////////////////////////////////////////

ezStatus ezRmlUiAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag,
  const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader,
  ezBitflags<ezTransformFlags> transformFlags)
{
  ezRmlUiAssetProperties* pProp = GetProperties();

  ezRmlUiResourceDescriptor desc;
  desc.m_sRmlFile = pProp->m_sRmlFile;

  // TODO dependencies
  
  desc.Save(stream);

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezRmlUiAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  ezStatus status = ezAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
