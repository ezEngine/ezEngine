#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Core/Graphics/AmbientCubeBasis.h>
#include <Foundation/Threading/TaskSystem.h>

class ezTracerInterface;

namespace ezBakingInternal
{
  class EZ_BAKINGPLUGIN_DLL SkyVisibilityTask : public ezTask
  {
  public:
    SkyVisibilityTask(ezTracerInterface* pTracer, ezArrayPtr<const ezVec3> probePositions);
    ~SkyVisibilityTask();

    virtual void Execute() override;

    ezArrayPtr<const ezAmbientCube<ezUInt8>> GetSkyVisibility() const { return m_SkyVisibility; }

  private:
    ezTracerInterface* m_pTracer = nullptr;
    ezArrayPtr<const ezVec3> m_ProbePositions;

    ezDynamicArray<ezAmbientCube<ezUInt8>> m_SkyVisibility;
  };
} // namespace ezBakingInternal
