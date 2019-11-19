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
  float fProbeSpacing = 4.0f;

  m_vGridOrigin.x = ezMath::RoundDown(m_Bounds.m_vMin.x, fProbeSpacing);
  m_vGridOrigin.y = ezMath::RoundDown(m_Bounds.m_vMin.y, fProbeSpacing);
  m_vGridOrigin.z = ezMath::RoundDown(m_Bounds.m_vMin.z, fProbeSpacing);

  const ezVec3 vMax = m_Bounds.m_vMax + ezVec3(fProbeSpacing);

  for (float z = m_vGridOrigin.z; z < vMax.z; z += fProbeSpacing)
  {
    for (float y = m_vGridOrigin.y; y < vMax.y; y += fProbeSpacing)
    {
      for (float x = m_vGridOrigin.x; x < vMax.x; x += fProbeSpacing)
      {
        m_ProbePositions.PushBack(ezVec3(x, y, z));
      }
    }
  }
}
