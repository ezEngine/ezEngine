#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/Threading/TaskSystem.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

struct ezBakingSettings;
class ezTracerInterface;

namespace ezBakingInternal
{
  class EZ_BAKINGPLUGIN_DLL SkyVisibilityTask : public ezTask
  {
  public:
    SkyVisibilityTask(const ezBakingSettings& settings, ezTracerInterface& tracer, ezArrayPtr<const ezVec3> probePositions);
    ~SkyVisibilityTask();

    virtual void Execute() override;

    ezArrayPtr<const ezCompressedSkyVisibility> GetSkyVisibility() const { return m_SkyVisibility; }

  private:
    const ezBakingSettings& m_Settings;

    ezTracerInterface& m_Tracer;
    ezArrayPtr<const ezVec3> m_ProbePositions;

    ezDynamicArray<ezCompressedSkyVisibility> m_SkyVisibility;
  };
} // namespace ezBakingInternal
