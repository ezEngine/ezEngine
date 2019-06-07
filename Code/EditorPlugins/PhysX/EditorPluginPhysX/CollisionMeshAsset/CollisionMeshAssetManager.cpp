#include <EditorPluginPhysXPCH.h>

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

  m_AssetDesc.m_bCanCreate = true;
  m_AssetDesc.m_sDocumentTypeName = "Collision Mesh Asset";
  m_AssetDesc.m_sFileExtension = "ezCollisionMeshAsset";
  m_AssetDesc.m_sIcon = ":/AssetIcons/Collision_Mesh.png";
  m_AssetDesc.m_pDocumentType = ezGetStaticRTTI<ezCollisionMeshAssetDocument>();
  m_AssetDesc.m_pManager = this;

  m_ConvexAssetDesc.m_bCanCreate = true;
  m_ConvexAssetDesc.m_sDocumentTypeName = "Collision Mesh Asset (Convex)";
  m_ConvexAssetDesc.m_sFileExtension = "ezConvexCollisionMeshAsset";
  m_ConvexAssetDesc.m_sIcon = ":/AssetIcons/Collision_Mesh_Convex.png";
  m_ConvexAssetDesc.m_pDocumentType = ezGetStaticRTTI<ezCollisionMeshAssetDocument>();
  m_ConvexAssetDesc.m_pManager = this;
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
            new ezQtCollisionMeshAssetDocumentWindow(static_cast<ezAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

ezStatus ezCollisionMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath,
                                                                     bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  if (ezStringUtils::IsEqual(szDocumentTypeName, "Collision Mesh Asset (Convex)"))
  {
    out_pDocument = new ezCollisionMeshAssetDocument(szPath, true);
  }
  else
  {
    out_pDocument = new ezCollisionMeshAssetDocument(szPath, false);
  }
  return ezStatus(EZ_SUCCESS);
}

void ezCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(
    ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_AssetDesc);
  inout_DocumentTypes.PushBack(&m_ConvexAssetDesc);
}

ezBitflags<ezAssetDocumentFlags>
ezCollisionMeshAssetDocumentManager::GetAssetDocumentTypeFlags(const ezDocumentTypeDescriptor* pDescriptor) const
{
  EZ_ASSERT_DEBUG(pDescriptor->m_pManager == this, "Given type descriptor is not part of this document manager!");
  return ezAssetDocumentFlags::SupportsThumbnail;
}

ezUInt64 ezCollisionMeshAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
