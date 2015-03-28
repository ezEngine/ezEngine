#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjectsManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetDocument, ezAssetDocument, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezTextureAssetDocument::ezTextureAssetDocument(const char* szDocumentPath) : ezAssetDocument(szDocumentPath, EZ_DEFAULT_NEW(ezTextureAssetObjectManager))
{
}

ezTextureAssetDocument::~ezTextureAssetDocument()
{
}

const ezTextureAssetProperties* ezTextureAssetDocument::GetProperties() const
{
  ezTextureAssetObject* pObject = (ezTextureAssetObject*) GetObjectTree()->GetRootObject()->GetChildren()[0];
  return &pObject->m_MemberProperties;
}

void ezTextureAssetDocument::Initialize()
{
  ezAssetDocument::Initialize();

  EnsureSettingsObjectExist();
}

void ezTextureAssetDocument::EnsureSettingsObjectExist()
{
  auto pRoot = GetObjectTree()->GetRootObject();
  if (pRoot->GetChildren().IsEmpty())
  {
    ezTextureAssetObject* pObject = static_cast<ezTextureAssetObject*>(GetObjectManager()->CreateObject(ezReflectedTypeManager::GetTypeHandleByName(ezGetStaticRTTI<ezTextureAssetProperties>()->GetTypeName())));

    GetObjectTree()->AddObject(pObject, pRoot);
  }

}

ezStatus ezTextureAssetDocument::InternalLoadDocument()
{
  GetObjectTree()->DestroyAllObjects(GetObjectManager());

  ezStatus ret = ezAssetDocument::InternalLoadDocument();

  EnsureSettingsObjectExist();

  return ret;
}

void ezTextureAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo)
{
  pInfo->m_FileDependencies.Clear();

  const ezTextureAssetProperties* pProp = GetProperties();
  pInfo->m_FileDependencies.PushBack(pProp->GetInputFile());

  pInfo->m_uiSettingsHash = GetDocumentHash();
}
