#pragma once

#include <EditorFramework/EngineProcess/EngineProcessViewContext.h>

class ezParticleContext;

class ezParticleViewContext : public ezEngineProcessViewContext
{
public:
  ezParticleViewContext(ezParticleContext* pParticleContext);
  ~ezParticleViewContext();

  void PositionThumbnailCamera();

protected:
  virtual ezViewHandle CreateView() override;

  ezParticleContext* m_pParticleContext;
};

