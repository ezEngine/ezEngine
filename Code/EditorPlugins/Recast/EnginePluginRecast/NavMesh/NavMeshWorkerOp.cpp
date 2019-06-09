#include <EnginePluginRecastPCH.h>

#include <EnginePluginRecast/NavMesh/NavMeshWorkerOp.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <ToolsFoundation/Document/DocumentManager.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOpWorker_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOpWorker_BuildNavMesh>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLongOpWorker_BuildNavMesh::InitializeExecution(const ezUuid& DocumentGuid)
{
  ezEngineProcessDocumentContext* pDocContext = ezEngineProcessDocumentContext::GetDocumentContext(DocumentGuid);

  if (pDocContext == nullptr)
    return EZ_FAILURE;

  pDocContext->GetWorld();

  ezRecastConfig config;

  EZ_LOCK(pDocContext->GetWorld()->GetWriteMarker());

  ezRecastNavMeshResourceDescriptor desc;

  ezRecastNavMeshBuilder NavMeshBuilder;
  EZ_SUCCEED_OR_RETURN(NavMeshBuilder.Build(config, *pDocContext->GetWorld(), desc));

  return EZ_SUCCESS;
}

void ezLongOpWorker_BuildNavMesh::Execute(const ezTask* pExecutingTask)
{
  for (ezUInt32 i = 0; i < 100; ++i)
  {
    SetCompletion(i / 100.0f);
    ezThreadUtils::Sleep(ezTime::Milliseconds(50));
  }
}

void ezLongOpWorker_BuildNavMesh::InitializeReplicated(ezStreamReader& description)
{
  description >> m_sOutputPath;
}
