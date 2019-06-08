#include <EnginePluginRecastPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <ToolsFoundation/Document/DocumentManager.h>

class ezLongOperationLocal_BuildNavMesh : public ezLongOperationLocal
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationLocal_BuildNavMesh, ezLongOperationLocal);

public:
  virtual const char* GetDisplayName() const override { return "Generate NavMesh"; }
  virtual void InitializeReplicated(ezStreamReader& description) override;
  virtual ezResult InitializeExecution(const ezUuid& DocumentGuid) override;
  virtual void Execute(const ezTask* pExecutingTask) override;
};

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOperationLocal_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOperationLocal_BuildNavMesh>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLongOperationLocal_BuildNavMesh::InitializeExecution(const ezUuid& DocumentGuid)
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

void ezLongOperationLocal_BuildNavMesh::Execute(const ezTask* pExecutingTask)
{
  for (ezUInt32 i = 0; i < 100; ++i)
  {
    SetCompletion(i / 100.0f);
    ezThreadUtils::Sleep(ezTime::Milliseconds(50));
  }
}

void ezLongOperationLocal_BuildNavMesh::InitializeReplicated(ezStreamReader& description)
{
  //
}
