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
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh.svg";
  m_DocTypeDesc.m_sAssetCategory = "Physics";
  m_DocTypeDesc.m_pDocumentType = ezGetStaticRTTI<ezJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle");

  m_DocTypeDesc.m_sResourceFileExtension = "ezBinJoltTriangleMesh";
  m_DocTypeDesc.m_AssetDocumentFlags = ezAssetDocumentFlags::SupportsThumbnail;

  m_DocTypeDesc2.m_sDocumentTypeName = "Jolt_Colmesh_Convex";
  m_DocTypeDesc2.m_sFileExtension = "ezJoltConvexCollisionMeshAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Jolt_Collision_Mesh_Convex.svg";
  m_DocTypeDesc2.m_sAssetCategory = "Physics";
  m_DocTypeDesc2.m_pDocumentType = ezGetStaticRTTI<ezJoltCollisionMeshAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Triangle"); // convex meshes can also be used as triangle meshes (concave)
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Jolt_Colmesh_Convex");

  m_DocTypeDesc2.m_sResourceFileExtension = "ezBinJoltConvexMesh";
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
        new ezQtJoltCollisionMeshAssetDocumentWindow(static_cast<ezAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;
    default:
      break;
  }
}

void ezJoltCollisionMeshAssetDocumentManager::InternalCreateDocument(ezStringView sDocumentTypeName, ezStringView sPath, bool bCreateNewDocument, ezDocument*& out_pDocument, const ezDocumentObject* pOpenContext)
{
  if (sDocumentTypeName.IsEqual("Jolt_Colmesh_Convex"))
  {
    out_pDocument = new ezJoltCollisionMeshAssetDocument(sPath, true);
  }
  else
  {
    out_pDocument = new ezJoltCollisionMeshAssetDocument(sPath, false);
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
