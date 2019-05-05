#include <ProceduralPlacementPluginPCH.h>

#include <ProceduralPlacementPlugin/Tasks/PrepareTask.h>

using namespace ezPPInternal;

PrepareTask::PrepareTask(const char* szName)
  : ezTask(szName)
{
}

PrepareTask::~PrepareTask() = default;

void PrepareTask::Execute()
{
  // Nothing to do here atm
}
