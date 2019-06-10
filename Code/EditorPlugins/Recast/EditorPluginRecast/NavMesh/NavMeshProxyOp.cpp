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

  {
    ezStringBuilder sComponentGuid, sOutputFile;
    ezConversionUtils::ToString(m_ComponentGuid, sComponentGuid);

    sOutputFile.Format(":project/AssetCache/Generated/{0}.ezRecastNavMesh", sComponentGuid);

    description << sOutputFile;
  }

  // TODO: pass along navmesh settings
  // ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(m_DocumentGuid);
  // ezDocumentObject* pObject =  pDoc->GetObjectManager()->GetObject(m_ComponentGuid);
  // pObject->GetTypeAccessor().GetValue("");
}

void ezLongOpProxy_BuildNavMesh::Finalize(ezResult result, const ezDataBuffer& resultData)
{
  if (result.Succeeded())
  {
    // TODO: reload navmesh resource ?
  }
}
