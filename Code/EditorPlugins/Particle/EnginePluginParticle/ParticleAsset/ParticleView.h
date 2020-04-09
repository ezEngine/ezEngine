#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessViewContext.h>

class ezParticleContext;

class ezParticleViewContext : public ezEngineProcessViewContext
{
public:
  ezParticleViewContext(ezParticleContext* pParticleContext);
  ~ezParticleViewContext();

  void PositionThumbnailCamera(const ezBoundingBoxSphere& bounds);

protected:
  virtual ezViewHandle CreateView() override;

  ezParticleContext* m_pParticleContext;
};

