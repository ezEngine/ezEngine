#include <PCH.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAsset.h>
#include <EditorPluginAssets/SurfaceAsset/SurfaceAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Image/Image.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSurfaceAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSurfaceAssetDocument::ezSurfaceAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezSurfaceResourceDescriptor>(szDocumentPath)
{
}

void ezSurfaceAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  ezAssetDocument::UpdateAssetDocumentInfo(pInfo);

  const ezSurfaceResourceDescriptor* pProp = GetProperties();

}

ezStatus ezSurfaceAssetDocument::InternalTransformAsset(ezStreamWriter& stream, const char* szPlatform)
{
  ezResourceHandleWriteContext writer;
  writer.BeginWritingToStream(&stream);

  const ezSurfaceResourceDescriptor* pProp = GetProperties();

  pProp->Save(stream);

  writer.EndWritingToStream(&stream);

  return ezStatus(EZ_SUCCESS);
}

