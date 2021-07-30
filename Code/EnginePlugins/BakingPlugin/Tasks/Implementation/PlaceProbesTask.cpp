#include <BakingPluginPCH.h>

#include <BakingPlugin/Tasks/PlaceProbesTask.h>

using namespace ezBakingInternal;

PlaceProbesTask::PlaceProbesTask(const ezBoundingBox& bounds, const ezVec3& probeSpacing)
  : m_Bounds(bounds)
  , m_vProbeSpacing(probeSpacing)
{
}

PlaceProbesTask::~PlaceProbesTask() = default;

void PlaceProbesTask::Execute()
{
  m_vGridOrigin.x = ezMath::RoundDown(m_Bounds.m_vMin.x, m_vProbeSpacing.x);
  m_vGridOrigin.y = ezMath::RoundDown(m_Bounds.m_vMin.y, m_vProbeSpacing.y);
  m_vGridOrigin.z = ezMath::RoundDown(m_Bounds.m_vMin.z, m_vProbeSpacing.z);

  const ezVec3 vMax = m_Bounds.m_vMax + m_vProbeSpacing;

  m_vProbeCount.x = static_cast<ezUInt32>(ezMath::Ceil((vMax.x - m_vGridOrigin.x) / m_vProbeSpacing.x));
  m_vProbeCount.y = static_cast<ezUInt32>(ezMath::Ceil((vMax.y - m_vGridOrigin.y) / m_vProbeSpacing.y));
  m_vProbeCount.z = static_cast<ezUInt32>(ezMath::Ceil((vMax.z - m_vGridOrigin.z) / m_vProbeSpacing.z));

  for (float z = m_vGridOrigin.z; z < vMax.z; z += m_vProbeSpacing.z)
  {
    for (float y = m_vGridOrigin.y; y < vMax.y; y += m_vProbeSpacing.y)
    {
      for (float x = m_vGridOrigin.x; x < vMax.x; x += m_vProbeSpacing.x)
      {
        m_ProbePositions.PushBack(ezVec3(x, y, z));
      }
    }
  }

  EZ_ASSERT_DEBUG(m_ProbePositions.GetCount() == m_vProbeCount.x * m_vProbeCount.y * m_vProbeCount.z, "Implementation error");
}
