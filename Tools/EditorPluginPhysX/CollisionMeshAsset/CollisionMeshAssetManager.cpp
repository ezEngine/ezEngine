#include <PCH.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetManager.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetWindow.moc.h>
#include "ToolsFoundation/Assets/AssetFileExtensionWhitelist.h"

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCollisionMeshAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezCollisionMeshAssetDocumentManager::ezCollisionMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  //ezAssetFileExtensionWhitelist::AddAssetFileExtension("Collision Mesh", "ezPhysXMesh");

}

ezCollisionMeshAssetDocumentManager::~ezCollisionMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezCollisionMeshAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
  case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezCollisionMeshAssetDocument>())
      {
        ezCollisionMeshAssetDocumentWindow* pDocWnd = new ezCollisionMeshAssetDocumentWindow(e.m_pDocument);
      }
    }
    break;
  }
}

ezStatus ezCollisionMeshAssetDocumentManager::InternalCanOpenDocument(const char* szDocumentTypeName, const char* szFilePath) const
{
  return EZ_SUCCESS;
}

ezStatus ezCollisionMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, ezDocument*& out_pDocument)
{
  out_pDocument = new ezCollisionMeshAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(ezHybridArray<ezDocumentTypeDescriptor, 4>& out_DocumentTypes) const
{
  {
    ezDocumentTypeDescriptor td;
    td.m_bCanCreate = true;
    td.m_sDocumentTypeName = "Collision Mesh Asset";
    td.m_sFileExtensions.PushBack("ezCollisionMeshAsset");
    td.m_sIcon = ":/AssetIcons/Collision_Mesh.png";

    out_DocumentTypes.PushBack(td);
  }
}



