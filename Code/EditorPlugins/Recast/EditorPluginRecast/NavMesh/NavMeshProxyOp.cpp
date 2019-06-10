#include <EditorPluginRecastPCH.h>

#include <EditorPluginRecast/NavMesh/NavMeshProxyOp.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOpProxy_BuildNavMesh>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLongOpProxy_BuildNavMesh::InitializeRegistered(const ezUuid& documentGuid, const ezUuid& componentGuid)
{
  m_DocumentGuid = documentGuid;
  m_ComponentGuid = componentGuid;
}

void ezLongOpProxy_BuildNavMesh::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOpWorker_BuildNavMesh";

  ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(m_DocumentGuid);

  ezStringBuilder sOutputFile = pDoc->GetDocumentPath();
  sOutputFile.RemoveFileExtension();
  sOutputFile.Append("_data/Scene.ezRecastNavMesh");

  description << sOutputFile;

  // TODO: pass along navmesh settings
  // ezDocumentObject* pObject =  pDoc->GetObjectManager()->GetObject(m_ComponentGuid);
  // pObject->GetTypeAccessor().GetValue("");
}
