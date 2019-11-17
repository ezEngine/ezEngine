#pragma once

#include <BakingPlugin/BakingPluginDLL.h>

class ezBakingScene;

class EZ_BAKINGPLUGIN_DLL ezTracerInterface
{
public:
  virtual ezResult BuildScene(const ezBakingScene& scene) = 0;

  struct Ray
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vStartPos;
    ezVec3 m_vDir;
    float m_fDistance;
  };

  struct Hit
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    ezVec3 m_vNormal;
    float m_fDistance;
  };

  virtual void TraceRays(ezArrayPtr<const Ray> rays, ezArrayPtr<Hit> hits) = 0;
};
