#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>

class ezPhysicsWorldModuleInterface;

/// Factory for volume finalizers.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory_Volume final : public ezParticleFinalizerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory_Volume, ezParticleFinalizerFactory);

public:
  ezParticleFinalizerFactory_Volume();
  ~ezParticleFinalizerFactory_Volume();

  virtual const ezRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const override;
};


/// Computes the bounding volume for the entire particle system.
///
/// Calculates a bounding box-sphere from all particle positions and optionally considers
/// particle sizes to determine the maximum extent. The computed volume is used for culling
/// and rendering optimizations. If no particles exist, the process is skipped.
class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_Volume final : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_Volume, ezParticleFinalizer);

public:
  ezParticleFinalizer_Volume();
  ~ezParticleFinalizer_Volume();

  virtual void CreateRequiredStreams() override;
  virtual void QueryOptionalStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  const ezProcessingStream* m_pStreamSize = nullptr;
};
