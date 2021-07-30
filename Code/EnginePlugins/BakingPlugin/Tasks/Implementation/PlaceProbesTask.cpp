#include <BakingPluginPCH.h>

#include <BakingPlugin/Tasks/PlaceProbesTask.h>

using namespace ezBakingInternal;

PlaceProbesTask::PlaceProbesTask(const ezBoundingBox& bounds)
  : m_Bounds(bounds)
{
}

PlaceProbesTask::~PlaceProbesTask() = default;

void PlaceProbesTask::Execute()
{
  m_vGridOrigin.x = ezMath::RoundDown(m_Bounds.m_vMin.x, ProbeSpacing);
  m_vGridOrigin.y = ezMath::RoundDown(m_Bounds.m_vMin.y, ProbeSpacing);
  m_vGridOrigin.z = ezMath::RoundDown(m_Bounds.m_vMin.z, ProbeSpacing);

  const ezVec3 vMax = m_Bounds.m_vMax + ezVec3(ProbeSpacing);

  m_vProbeCount.x = static_cast<ezUInt32>(ezMath::Ceil((vMax.x - m_vGridOrigin.x) / ProbeSpacing));
  m_vProbeCount.y = static_cast<ezUInt32>(ezMath::Ceil((vMax.y - m_vGridOrigin.y) / ProbeSpacing));
  m_vProbeCount.z = static_cast<ezUInt32>(ezMath::Ceil((vMax.z - m_vGridOrigin.z) / ProbeSpacing));

  for (float z = m_vGridOrigin.z; z < vMax.z; z += ProbeSpacing)
  {
    for (float y = m_vGridOrigin.y; y < vMax.y; y += ProbeSpacing)
    {
      for (float x = m_vGridOrigin.x; x < vMax.x; x += ProbeSpacing)
      {
        m_ProbePositions.PushBack(ezVec3(x, y, z));
      }
    }
  }

  EZ_ASSERT_DEBUG(m_ProbePositions.GetCount() == m_vProbeCount.x * m_vProbeCount.y * m_vProbeCount.z, "Implementation error");
}
