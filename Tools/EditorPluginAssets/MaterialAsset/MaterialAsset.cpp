#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjects.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocument, ezAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialAssetDocument::ezMaterialAssetDocument(const char* szDocumentPath) : ezSimpleAssetDocument<ezMaterialAssetProperties, ezMaterialAssetObject, ezMaterialAssetObjectManager>(szDocumentPath)
{
}

void ezMaterialAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  const ezMaterialAssetProperties* pProp = GetProperties();

  //ezStringBuilder sTemp = pProp->GetInputFile();
  //sTemp.MakeCleanPath();

  // TODO

  //pInfo->m_FileDependencies.PushBack(sTemp);
}

ezStatus ezMaterialAssetDocument::InternalTransformAsset(ezStreamWriterBase& stream, const char* szPlatform)
{
  // TODO
  //const ezImage* pImage = &GetProperties()->GetImage();
  //SaveThumbnail(*pImage);

  return ezStatus(EZ_SUCCESS);
}

