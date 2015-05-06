#include <PCH.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjects.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetManager.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetObjectsManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMaterialAssetDocument, ezAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezMaterialAssetDocument::ezMaterialAssetDocument(const char* szDocumentPath) : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezMaterialAssetObjectManager))
{
}

ezMaterialAssetDocument::~ezMaterialAssetDocument()
{
}

const ezMaterialAssetProperties* ezMaterialAssetDocument::GetProperties() const
{
  ezMaterialAssetObject* pObject = (ezMaterialAssetObject*) GetObjectTree()->GetRootObject()->GetChildren()[0];
  return &pObject->m_MemberProperties;
}

void ezMaterialAssetDocument::Initialize()
{
  ezAssetDocument::Initialize();

  EnsureSettingsObjectExist();
}

void ezMaterialAssetDocument::EnsureSettingsObjectExist()
{
  auto pRoot = GetObjectTree()->GetRootObject();
  if (pRoot->GetChildren().IsEmpty())
  {
    ezMaterialAssetObject* pObject = static_cast<ezMaterialAssetObject*>(GetObjectManager()->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezMaterialAssetProperties>()->GetTypeName())));

    GetObjectTree()->AddObject(pObject, pRoot);
  }

}

ezStatus ezMaterialAssetDocument::InternalLoadDocument()
{
  GetObjectTree()->DestroyAllObjects(GetObjectManager());

  ezStatus ret = ezAssetDocument::InternalLoadDocument();

  EnsureSettingsObjectExist();

  return ret;
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

const char* ezMaterialAssetDocument::QueryAssetType() const
{
  return "Material";
}