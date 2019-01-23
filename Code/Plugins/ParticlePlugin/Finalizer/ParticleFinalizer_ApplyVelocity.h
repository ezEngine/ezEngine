#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory_ApplyVelocity : public ezParticleFinalizerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory_ApplyVelocity, ezParticleFinalizerFactory);

public:
  ezParticleFinalizerFactory_ApplyVelocity();

  virtual const ezRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const override;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_ApplyVelocity : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_ApplyVelocity, ezParticleFinalizer);
public:
  ezParticleFinalizer_ApplyVelocity();
  ~ezParticleFinalizer_ApplyVelocity();

  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
};
