#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentManager, 1, ezRTTIDefaultAllocator<ezSceneDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneDocumentManager* ezSceneDocumentManager::s_pSingleton = nullptr;

ezStatus ezSceneDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  ezStatus status;

  if (ezStringUtils::IsEqual(szDocumentTypeName, "ezScene"))
  {
    out_pDocument = new ezSceneDocument(szPath, false);
  }
  else
  if (ezStringUtils::IsEqual(szDocumentTypeName, "ezPrefab"))
  {
    out_pDocument = new ezSceneDocument(szPath, true);
  }
  else
  {
    status.m_sError = "Unknown Document Type";
  }

  if (out_pDocument)
  {
    status.m_Result = EZ_SUCCESS;
    //out_pDocument->SetFilePath(szPath);
  }

  return status;
}

void ezSceneDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "ezScene";
    td.m_sFileExtensions.PushBack("ezScene");
    td.m_sIcon = ":/AssetIcons/Scene.png";

    out_DocumentTypes.PushBack(td);
  }

  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "ezPrefab";
    td.m_sFileExtensions.PushBack("ezPrefab");
    td.m_sIcon = ":/AssetIcons/Prefab.png";

    out_DocumentTypes.PushBack(td);
  }
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


