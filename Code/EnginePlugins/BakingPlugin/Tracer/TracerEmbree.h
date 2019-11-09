#pragma once

#include <BakingPlugin/Tracer/TracerInterface.h>
#include <Foundation/Types/UniquePtr.h>

class EZ_BAKINGPLUGIN_DLL ezTracerEmbree : public ezTracerInterface
{
public:
  ezTracerEmbree();
  ~ezTracerEmbree();

  virtual ezResult BuildScene(const ezBaking::Scene& scene) override;

  virtual void TraceRays() override;

private:
  struct Data;

  ezUniquePtr<Data> m_pData;
};
