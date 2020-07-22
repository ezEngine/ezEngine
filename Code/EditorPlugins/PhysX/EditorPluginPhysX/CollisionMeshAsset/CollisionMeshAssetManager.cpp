#include <EditorPluginPhysXPCH.h>

#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAsset.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetManager.h>
#include <EditorPluginPhysX/CollisionMeshAsset/CollisionMeshAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCollisionMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezCollisionMeshAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezCollisionMeshAssetDocumentManager::ezCollisionMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Collision Mesh";
  m_DocTypeDesc.m_sFileExtension = "ezCollisionMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Collision_Mesh.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezCollisionMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezPhysXMesh";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;

  m_DocTypeDesc2.m_sDocumentTypeName = "Collision Mesh (Convex)";
  m_DocTypeDesc2.m_sFileExtension = "ezConvexCollisionMeshAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Collision_Mesh_Convex.png";
  m_DocTypeDesc2.m_pDocumentType = ezGetStaticRTTI<ezCollisionMeshAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;

  m_DocTypeDesc2.m_sResourceFileExtension = "ezPhysXMesh";
  m_DocTypeDesc2.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
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
        ezQtCollisionMeshAssetDocumentWindow* pDocWnd = new ezQtCollisionMeshAssetDocumentWindow(static_cast<ezAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

void ezCollisionMeshAssetDocumentManager::InternalCreateDocument(
  const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument)
{
  if (ezStringUtils::IsEqual(szDocumentTypeName, "Collision Mesh (Convex)"))
  {
    out_pDocument = new ezCollisionMeshAssetDocument(szPath, true);
  }
  else
  {
    out_pDocument = new ezCollisionMeshAssetDocument(szPath, false);
  }
}

void ezCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(
  ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

ezUInt64 ezCollisionMeshAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
