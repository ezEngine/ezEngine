#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentManager, 1, ezRTTIDefaultAllocator<ezSceneDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE

ezSceneDocumentManager* ezSceneDocumentManager::s_pSingleton = nullptr;


ezSceneDocumentManager::ezSceneDocumentManager()
{
  s_pSingleton = this;

  {
    m_SceneDesc.m_bCanCreate = true;
    m_SceneDesc.m_sDocumentTypeName = "Scene";
    m_SceneDesc.m_sFileExtension = "ezScene";
    m_SceneDesc.m_sIcon = ":/AssetIcons/Scene.png";
    m_SceneDesc.m_pDocumentType = ezGetStaticRTTI<ezSceneDocument>();
    m_SceneDesc.m_pManager = this;
  }

  {
    m_PrefabDesc.m_bCanCreate = true;
    m_PrefabDesc.m_sDocumentTypeName = "Prefab";
    m_PrefabDesc.m_sFileExtension = "ezPrefab";
    m_PrefabDesc.m_sIcon = ":/AssetIcons/Prefab.png";
    m_PrefabDesc.m_pDocumentType = ezGetStaticRTTI<ezSceneDocument>();
    m_PrefabDesc.m_pManager = this;
  }

  ezQtImageCache::GetSingleton()->RegisterTypeImage("Scene", QPixmap(":/AssetIcons/Scene.png"));
}


ezBitflags<ezAssetDocumentFlags> ezSceneDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  if (pDescriptor == &m_PrefabDesc)
  {
    return ezAssetDocumentFlags::AutoTransformOnSave | ezAssetDocumentFlags::SupportsThumbnail;
  }
  else
  {
    return ezAssetDocumentFlags::OnlyTransformManually;
  }
}

ezStatus ezSceneDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  ezStatus status;

  if (ezStringUtils::IsEqual(szDocumentTypeName, "Scene"))
  {
    out_pDocument = new ezSceneDocument(szPath, false);
  }
  else
  if (ezStringUtils::IsEqual(szDocumentTypeName, "Prefab"))
  {
    out_pDocument = new ezSceneDocument(szPath, true);
  }
  else
  {
    status.m_sMessage = "Unknown Document Type";
  }

  if (out_pDocument)
  {
    status.m_Result = EZ_SUCCESS;
    //out_pDocument->SetFilePath(szPath);
  }

  return status;
}

void ezSceneDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_SceneDesc);
  inout_DocumentTypes.PushBack(&m_PrefabDesc);
}

ezString ezSceneDocumentManager::GetResourceTypeExtension() const
{
  return "ezObjectGraph";
}

void ezSceneDocumentManager::QuerySupportedAssetTypes(ezSet<ezString>& inout_AssetTypeNames) const
{
  inout_AssetTypeNames.Insert("Scene");
  inout_AssetTypeNames.Insert("Prefab");
}


