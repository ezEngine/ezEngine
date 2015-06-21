#include <PCH.h>
#include <EditorPluginScene/Scene/SceneDocumentManager.h>
#include <EditorPluginScene/Scene/SceneDocument.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezSceneDocumentManager, ezDocumentManagerBase, 1, ezRTTIDefaultAllocator<ezSceneDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezSceneDocumentManager* ezSceneDocumentManager::s_pSingleton = nullptr;

ezStatus ezSceneDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezSceneDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocumentBase*& out_pDocument)
{
  ezStatus status;

  if (ezStringUtils::IsEqual(szDocumentTypeName, "ezScene"))
  {
    out_pDocument = new ezSceneDocument(szPath);
  }
  //else
  //if (ezStringUtils::IsEqual(szDocumentTypeName, "ezPrefab"))
  //{
  //  out_pDocument = new ezSceneDocument(szPath);
  //}
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
    td.m_sIcon = ":/GuiFoundation/Icons/ezEditor16.png";

    out_DocumentTypes.PushBack(td);
  }
}



