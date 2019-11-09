#pragma once

#include <BakingPlugin/Baking.h>

class EZ_BAKINGPLUGIN_DLL ezTracerInterface
{
public:
  virtual ezResult BuildScene(const ezBaking::Scene& scene) = 0;

  struct Ray
  {
    ezVec3 m_vOrigin;
  };

  virtual void TraceRays() = 0;
};
