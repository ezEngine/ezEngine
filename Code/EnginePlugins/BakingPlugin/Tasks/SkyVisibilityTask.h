#pragma once

#include <BakingPlugin/BakingPluginDLL.h>
#include <Foundation/Threading/TaskSystem.h>
#include <RendererCore/BakedProbes/BakingUtils.h>

class ezTracerInterface;

namespace ezBakingInternal
{
  class EZ_BAKINGPLUGIN_DLL SkyVisibilityTask : public ezTask
  {
  public:
    SkyVisibilityTask(ezTracerInterface* pTracer, ezArrayPtr<const ezVec3> probePositions);
    ~SkyVisibilityTask();

    virtual void Execute() override;

    ezArrayPtr<const ezCompressedSkyVisibility> GetSkyVisibility() const { return m_SkyVisibility; }

  private:
    ezTracerInterface* m_pTracer = nullptr;
    ezArrayPtr<const ezVec3> m_ProbePositions;

    ezDynamicArray<ezCompressedSkyVisibility> m_SkyVisibility;
  };
} // namespace ezBakingInternal
