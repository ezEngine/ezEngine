#include <EnginePluginRecastPCH.h>

#include <EditorEngineProcessFramework/LongOperation/LongOperation.h>

class ezLongOperationLocal_BuildNavMesh : public ezLongOperationLocal
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLongOperationLocal_BuildNavMesh, ezLongOperationLocal);

public:
  virtual const char* GetDisplayName() const override { return "Generate NavMesh"; }
  virtual void Execute(const ezTask* pExecutingTask) override;
  virtual void InitializeReplicated(ezStreamReader& description) override;
};

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLongOperationLocal_BuildNavMesh, 1, ezRTTIDefaultAllocator<ezLongOperationLocal_BuildNavMesh>);
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

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

