#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetManager.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetWindow.moc.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltCollisionMeshAssetDocumentManager, 1, ezRTTIDefaultAllocator<ezJoltCollisionMeshAssetDocumentManager>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezJoltCollisionMeshAssetDocumentManager::ezJoltCollisionMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.AddEventHandler(ezMakeDelegate(&ezJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Jolt_Colmesh_Triangle";
  m_DocTypeDesc.m_sFileExtension = "ezJoltCollisionMeshAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh.png";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;

  m_DocTypeDesc.m_sResourceFileExtension = "ezJoltMesh";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;

  m_DocTypeDesc2.m_sDocumentTypeName = "Jolt_Colmesh_Convex";
  m_DocTypeDesc2.m_sFileExtension = "ezJoltConvexCollisionMeshAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh_Convex.png";
  m_DocTypeDesc2.m_pDocumentType = ezGetStaticRTTI<ezJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;

  m_DocTypeDesc2.m_sResourceFileExtension = "ezJoltMesh";
  m_DocTypeDesc2.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;
}

ezJoltCollisionMeshAssetDocumentManager::~ezJoltCollisionMeshAssetDocumentManager()
{
  ezDocumentManager::s_Events.RemoveEventHandler(ezMakeDelegate(&ezJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent, this));
}

void ezJoltCollisionMeshAssetDocumentManager::OnDocumentManagerEvent(const ezDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case ezDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == ezGetStaticRTTI<ezJoltCollisionMeshAssetDocument>())
      {
        ezQtJoltCollisionMeshAssetDocumentWindow* pDocWnd = new ezQtJoltCollisionMeshAssetDocumentWindow(static_cast<ezAssetDocument*>(e.m_pDocument));
      }
    }
    break;
  }
}

void ezJoltCollisionMeshAssetDocumentManager::InternalCreateDocument(const char* szDocumentTypeName, const char* szPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  if (ezStringUtils::IsEqual(szDocumentTypeName, "Jolt_Colmesh_Convex"))
  {
    out_pDocument = new ezJoltCollisionMeshAssetDocument(szPath, true);
  }
  else
  {
    out_pDocument = new ezJoltCollisionMeshAssetDocument(szPath, false);
  }
}

void ezJoltCollisionMeshAssetDocumentManager::InternalGetSupportedDocumentTypes(ezDynamicArray<const ezDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

ezUInt64 ezJoltCollisionMeshAssetDocumentManager::ComputeAssetProfileHashImpl(const ezPlatformProfile* pAssetProfile) const
{
  // don't have any settings yet, but assets that generate profile specific output must not return 0 here
  return 1;
}
