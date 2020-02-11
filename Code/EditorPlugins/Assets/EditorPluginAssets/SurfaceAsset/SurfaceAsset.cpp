#include <EditorPluginAssetsPCH.h>

#include <Core/WorldSerializer/ResourceHandleWriter.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetManager.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Image/Image.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezSurfaceAssetDocument::ezSurfaceAssetDocument(const char* szDocumentPath)
    : ezSimpleAssetDocument<ezSurfaceResourceDescriptor>(szDocumentPath, ezAssetDocEngineConnection::None)
{
}

ezStatus ezSurfaceAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                        const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezResourceHandleWriteContext writer;
  writer.BeginWritingToStream(&stream);

  const ezSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  writer.EndWritingToStream(&stream);

  return ezStatus(EZ_SUCCESS);
}
