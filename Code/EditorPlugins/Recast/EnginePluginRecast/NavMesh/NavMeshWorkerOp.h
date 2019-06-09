#include <EnginePluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>

class ezLongOpWorker_BuildNavMesh : public ezLongOpWorker
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOpWorker_BuildNavMesh, ezLongOpWorker);

public:
  virtual const char* GetDisplayName() const override { return "Generate NavMesh"; }
  virtual void InitializeReplicated(ezStreamReader& description) override;
  virtual ezResult InitializeExecution(const ezUuid& DocumentGuid) override;
  virtual void Execute(ezProgress& progress) override;

  ezString m_sOutputPath;
};
