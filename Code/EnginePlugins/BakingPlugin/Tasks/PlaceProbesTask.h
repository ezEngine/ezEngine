#pragma once

#include <BakingPlugin/Declarations.h>
#include <Foundation/Threading/TaskSystem.h>

struct ezBakingSettings;

namespace ezBakingInternal
{
  class EZ_BAKINGPLUGIN_DLL PlaceProbesTask : public ezTask
  {
  public:
    PlaceProbesTask(const ezBakingSettings& settings, const ezBoundingBox& bounds, ezArrayPtr<const Volume> volumes);
    ~PlaceProbesTask();

    virtual void Execute() override;

    ezArrayPtr<const ezVec3> GetProbePositions() const { return m_ProbePositions; }
    const ezVec3& GetGridOrigin() const { return m_vGridOrigin; }
    const ezVec3U32& GetProbeCount() const { return m_vProbeCount; }

  private:
    const ezBakingSettings& m_Settings;

    ezBoundingBox m_Bounds;
    ezArrayPtr<const Volume> m_Volumes;

    ezVec3 m_vGridOrigin = ezVec3::MakeZero();
    ezVec3U32 m_vProbeCount = ezVec3U32::MakeZero();
    ezDynamicArray<ezVec3> m_ProbePositions;
  };
} // namespace ezBakingInternal
