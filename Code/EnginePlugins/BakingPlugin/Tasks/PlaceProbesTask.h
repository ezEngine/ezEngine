#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/Threading/TaskSystem.h>

namespace ezBakingInternal
{
  class EZ_BAKINGPLUGIN_DLL PlaceProbesTask : public ezTask
  {
  public:
    PlaceProbesTask(const ezBoundingBox& bounds, const ezVec3& probeSpacing);
    ~PlaceProbesTask();

    virtual void Execute() override;

    ezArrayPtr<const ezVec3> GetProbePositions() const { return m_ProbePositions; }
    const ezVec3& GetGridOrigin() const { return m_vGridOrigin; }
    const ezVec3U32& GetProbeCount() const { return m_vProbeCount; }

  private:
    ezBoundingBox m_Bounds;
    ezVec3 m_vProbeSpacing;

    ezVec3 m_vGridOrigin = ezVec3::ZeroVector();
    ezVec3U32 m_vProbeCount = ezVec3U32::ZeroVector();
    ezDynamicArray<ezVec3> m_ProbePositions;
  };
} // namespace ezBakingInternal
