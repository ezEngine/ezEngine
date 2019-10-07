#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>

using namespace ezProcGenInternal;

PreparePlacementTask::PreparePlacementTask(PlacementData* pData, const char* szName)
  : ezTask(szName)
  , m_pData(pData)
{
}

PreparePlacementTask::~PreparePlacementTask() = default;

void PreparePlacementTask::Execute()
{
  const ezWorld& world = *m_pData->m_pPhysicsModule->GetWorld();
  ezBoundingBox box = m_pData->m_TileBoundingBox;
  const Output& output = *m_pData->m_pOutput;

  ezProcGenInternal::ExtractVolumeCollections(world, box, output, m_pData->m_VolumeCollections, m_pData->m_GlobalData);
}
