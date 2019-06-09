#include <EditorPluginRecastPCH.h>

#include <EditorPluginRecast/NavMesh/NavMeshProxyOp.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpProxy_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOpProxy_BuildNavMesh>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLongOpProxy_BuildNavMesh::GetReplicationInfo(ezStringBuilder& out_sReplicationOpType, ezStreamWriter& description)
{
  out_sReplicationOpType = "ezLongOpWorker_BuildNavMesh";

  description << m_sOutputPath;
}
