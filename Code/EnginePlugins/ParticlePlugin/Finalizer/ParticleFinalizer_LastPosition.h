#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

/// Factory for last position finalizers.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory_LastPosition final : public ezParticleFinalizerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory_LastPosition, ezParticleFinalizerFactory);

public:
  ezParticleFinalizerFactory_LastPosition();

  virtual const ezRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const override;
};


/// Records the previous frame's particle position for motion-based effects.
///
/// Copies the current position to the last position stream each frame. This data is used
/// by renderers to create motion blur trails or stretched particles. The finalizer has a
/// very low priority (-499) to run early in the frame, after initializers but before
/// most other processing.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_LastPosition final : public ezParticleFinalizer
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
