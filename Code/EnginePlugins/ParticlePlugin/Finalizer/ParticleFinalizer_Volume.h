#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>
#include <Foundation/Types/VarianceTypes.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory_Volume : public ezParticleFinalizerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory_Volume, ezParticleFinalizerFactory);

public:
  ezParticleFinalizerFactory_Volume();
  ~ezParticleFinalizerFactory_Volume();

  virtual const ezRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject, bool bFirstTime) const override;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_Volume : public ezParticleFinalizer
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
