#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceAssetDocument, 2, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSurfaceAssetDocument::ezSurfaceAssetDocument(ezStringView sDocumentPath)
  : ezSimpleAssetDocument<ezSurfaceResourceDescriptor>(sDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezTransformStatus ezSurfaceAssetDocument::InternalTransformAsset(ezStreamWriter& stream, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile,
  const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  const ezSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  return ezStatus(EZ_SUCCESS);
}
