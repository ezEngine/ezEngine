#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory_LastPosition : public ezParticleFinalizerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory_LastPosition, ezParticleFinalizerFactory);

public:
  ezParticleFinalizerFactory_LastPosition();

  virtual const ezRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject) const override;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_LastPosition : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_LastPosition, ezParticleFinalizer);
public:
  ezParticleFinalizer_LastPosition();
  ~ezParticleFinalizer_LastPosition();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamLastPosition = nullptr;
};
