#pragma once

#include <BakingPlugin/BakingPluginDLL.h>

class ezBakingScene;

class EZ_BAKINGPLUGIN_DLL ezTracerInterface
{
public:
  virtual ezResult BuildScene(const ezBakingScene& scene) = 0;

  struct Ray
  {
    ezVec3 m_vOrigin;
  };

  virtual void TraceRays() = 0;
};
