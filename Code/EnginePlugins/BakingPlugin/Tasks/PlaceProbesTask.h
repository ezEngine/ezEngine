#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/Threading/TaskSystem.h>

namespace ezBakingInternal
{
  class EZ_BAKINGPLUGIN_DLL PlaceProbesTask : public ezTask
  {
  public:
    PlaceProbesTask(const ezBoundingBox& bounds);
    ~PlaceProbesTask();

    virtual void Execute() override;

    ezArrayPtr<const ezVec3> GetProbePositions() const { return m_ProbePositions; }
    const ezVec3& GetGridOrigin() const { return m_vGridOrigin; }

  private:
    ezBoundingBox m_Bounds;

    ezVec3 m_vGridOrigin = ezVec3::ZeroVector();
    ezDynamicArray<ezVec3> m_ProbePositions;
  };
} // namespace ezBakingInternal
