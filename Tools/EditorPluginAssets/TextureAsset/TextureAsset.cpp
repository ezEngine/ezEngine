#include <PCH.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjects.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetObjectsManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeManager.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <CoreUtils/Image/Formats/DdsFileFormat.h>

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

  ezStringBuilder sTemp = pProp->GetInputFile();
  sTemp.MakeCleanPath();

  pInfo->m_FileDependencies.PushBack(sTemp);

  pInfo->m_uiSettingsHash = GetDocumentHash();
}

ezStatus ezTextureAssetDocument::InternalTransformAsset(ezStreamWriterBase& stream)
{
  const ezImage& img = GetProperties()->GetImage();

  stream << GetProperties()->IsSRGB();

  ezDdsFileFormat writer;
  if (writer.WriteImage(stream, img, ezGlobalLog::GetInstance()).Failed())
  {
    return ezStatus("Writing the image data as DDS failed");
  }

  return ezStatus(EZ_SUCCESS);
}

