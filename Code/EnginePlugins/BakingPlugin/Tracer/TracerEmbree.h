#pragma once

#include <BakingPlugin/Tracer/TracerInterface.h>
#include <Foundation/Types/UniquePtr.h>

class EZ_BAKINGPLUGIN_DLL ezTracerEmbree : public ezTracerInterface
{
public:
  ezTracerEmbree();
  ~ezTracerEmbree();

  virtual ezResult BuildScene(const ezBakingScene& scene) override;

  virtual void TraceRays(ezArrayPtr<const Ray> rays, ezArrayPtr<Hit> hits) override;

private:
  struct Data;

  ezUniquePtr<Data> m_pData;
};
