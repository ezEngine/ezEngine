#include <PCH.h>
#include <EditorPluginAssets/MeshAsset/MeshAsset.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetObjects.h>
#include <EditorPluginAssets/MeshAsset/MeshAssetManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <CoreUtils/Image/Image.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMeshAssetDocument, ezAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMeshAssetDocument::ezMeshAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezMeshAssetProperties, ezMeshAssetObject, ezMeshAssetObjectManager>(szDocumentPath)
{
}

void ezMeshAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezMeshAssetProperties* pProp = GetProperties();

  if (!pProp->m_sMeshFile.IsEmpty())
    pInfo->m_FileDependencies.PushBack(pProp->m_sMeshFile);

}

ezStatus ezMeshAssetDocument::InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform)
{
  const ezMeshAssetProperties* pProp = GetProperties();





  return ezStatus(EZ_SUCCESS);
}

