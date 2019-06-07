#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>

using namespace ezProcGenInternal;

PreparePlacementTask::PreparePlacementTask(const char* szName)
  : ezTask(szName)
{
}

PreparePlacementTask::~PreparePlacementTask() = default;

void PreparePlacementTask::Execute()
{
  // Nothing to do here atm
}
