#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Tasks/PreparePlacementTask.h>

using namespace ezProcGenInternal;

PreparePlacementTask::PreparePlacementTask(PlacementTask* placementTask, const char* szName)
  : ezTask(szName)
  , m_pPlacementTask(placementTask)
{
}

PreparePlacementTask::~PreparePlacementTask() = default;

void PreparePlacementTask::Execute()
{
  const ezWorld& world = *m_pPlacementTask->m_pPhysicsModule->GetWorld();
  ezBoundingBox box = m_pPlacementTask->m_TileBoundingBox;
  const Output& output = *m_pPlacementTask->m_pOutput;

  m_pPlacementTask->m_VolumeCollections.Clear();
  m_pPlacementTask->m_GlobalData.Clear();

  ezProcGenInternal::ExtractVolumeCollections(world, box, output, m_pPlacementTask->m_VolumeCollections,
    m_pPlacementTask->m_GlobalData);
}
