#include <ProcGenPluginPCH.h>

#include <ProcGenPlugin/Components/VolumeCollection.h>
#include <ProcGenPlugin/Tasks/PlacementData.h>

namespace ezProcGenInternal
{
  PlacementData::PlacementData() = default;
  PlacementData::~PlacementData() = default;

  void PlacementData::Clear()
  {
    m_pPhysicsModule = nullptr;

    m_pOutput = nullptr;
    m_iTileSeed = 0;
    m_TileBoundingBox.SetInvalid();
    m_GlobalToLocalBoxTransforms.Clear();

    m_VolumeCollections.Clear();
    m_GlobalData.Clear();
  }
} // namespace ezProcGenInternal
