#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Factory for apply velocity finalizers.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory_ApplyVelocity final : public ezParticleFinalizerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory_ApplyVelocity, ezParticleFinalizerFactory);

public:
  ezParticleFinalizerFactory_ApplyVelocity();

  virtual const ezRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const override;
};


/// Integrates particle velocity into position each frame.
///
/// Updates particle positions by adding velocity * time_delta. The velocity is stored as
/// a direction vector (xyz) and speed scalar (w). This finalizer has a slightly higher
/// priority (525) to run after most other finalizers.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_ApplyVelocity final : public ezParticleFinalizer
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
