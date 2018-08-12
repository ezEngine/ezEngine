#include <PCH.h>

#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetManager.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCollisionMeshAssetDocumentManager>);
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCollisionMeshAssetDocumentManager::ezCollisionMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  // ezAssetFileExtensionWhitelist::AddAssetFileExtension("Collision Mesh", "ezPhysXMesh");

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Collision Mesh Asset";
  m_AssetDesc.m_sFileExtension = "ezCollisionMeshAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Collision_Mesh.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezCollisionMeshAssetDocument>();
  m_AssetDesc.m_pManager = this;

  // ezQtImageCache::GetSingleton()->RegisterTypeImage("Collision Mesh", QPixmap(":/AssetIcons/Collision_Mesh.png"));
  // ezQtImageCache::GetSingleton()->RegisterTypeImage("Collision Mesh (Convex)", QPixmap(":/AssetIcons/Collision_Mesh.png"));
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
        ezQtCollisionMeshAssetDocumentWindow* pDocWnd =
            new ezQtCollisionMeshAssetDocumentWindow(static_cast<ezCollisionMeshAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezCollisionMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                     ezDocument*& out_pDocument)
{
  out_pDocument = new ezCollisionMeshAssetDocument(szPath);

  return ezStatus(EZ_SUCCESS);
}

void ezCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
}

ezBitflags<ezAssetDocumentFlags>
ezCollisionMeshAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::SupportsThumbnail;
}
