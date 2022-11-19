#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceAssetDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSurfaceAssetDocument::ezSurfaceAssetDocument(const char* szDocumentPath)
  : ezSimpleAssetDocument<ezSurfaceResourceDescriptor>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezTransformStatus ezSurfaceAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
  const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return ezStatus(EZ_SUCCESS);
}
