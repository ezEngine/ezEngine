#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/Tasks/PlaceProbesTask.h>
#include <RendererCore/BakedProbes/BakingInterface.h>

using namespace ezBakingInternal;

PlaceProbesTask::PlaceProbesTask(const ezBakingSettings& settings, const ezBoundingBox& bounds, ezArrayPtr<const Volume> volumes)
  : m_Settings(settings)
  , m_Bounds(bounds)
  , m_Volumes(volumes)
{
}

PlaceProbesTask::~PlaceProbesTask() = default;

void PlaceProbesTask::Execute()
{
  const ezVec3 probeSpacing = m_Settings.m_vProbeSpacing;

  m_vGridOrigin.x = ezMath::RoundDown(m_Bounds.m_vMin.x, probeSpacing.x);
  m_vGridOrigin.y = ezMath::RoundDown(m_Bounds.m_vMin.y, probeSpacing.y);
  m_vGridOrigin.z = ezMath::RoundDown(m_Bounds.m_vMin.z, probeSpacing.z);

  const ezVec3 vMax = m_Bounds.m_vMax + probeSpacing;

  m_vProbeCount.x = static_cast<ezUInt32>(ezMath::Ceil((vMax.x - m_vGridOrigin.x) / probeSpacing.x));
  m_vProbeCount.y = static_cast<ezUInt32>(ezMath::Ceil((vMax.y - m_vGridOrigin.y) / probeSpacing.y));
  m_vProbeCount.z = static_cast<ezUInt32>(ezMath::Ceil((vMax.z - m_vGridOrigin.z) / probeSpacing.z));

  for (float z = m_vGridOrigin.z; z < vMax.z; z += probeSpacing.z)
  {
    for (float y = m_vGridOrigin.y; y < vMax.y; y += probeSpacing.y)
    {
      for (float x = m_vGridOrigin.x; x < vMax.x; x += probeSpacing.x)
      {
        m_ProbePositions.PushBack(ezVec3(x, y, z));
      }
    }
  }

  EZ_ASSERT_DEBUG(m_ProbePositions.GetCount() == m_vProbeCount.x * m_vProbeCount.y * m_vProbeCount.z, "Implementation error");
}
