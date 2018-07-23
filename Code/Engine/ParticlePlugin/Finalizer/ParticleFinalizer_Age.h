#pragma once

#include <ParticlePlugin/Finalizer/ParticleFinalizer.h>
#include <Foundation/Types/VarianceTypes.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizerFactory_Age : public ezParticleFinalizerFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizerFactory_Age, ezParticleFinalizerFactory);

public:
  ezParticleFinalizerFactory_Age();

  virtual const ezRTTI* GetFinalizerType() const override;
  virtual void CopyFinalizerProperties(ezParticleFinalizer* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezVarianceTypeTime m_LifeTime;
  ezString m_sOnDeathEvent;
  ezString m_sLifeScaleParameter;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleFinalizer_Age : public ezParticleFinalizer
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleFinalizer_Age, ezParticleFinalizer);
public:
  ezParticleFinalizer_Age();
  ~ezParticleFinalizer_Age();

  virtual void CreateRequiredStreams() override;

  ezVarianceTypeTime m_LifeTime;
  ezTempHashedString m_sOnDeathEvent;
  ezTempHashedString m_sLifeScaleParameter;

  virtual void AfterPropertiesConfigured(bool bFirstTime) override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);

  bool m_bHasOnDeathEventHandler;
  ezProcessingStream* m_pStreamLifeTime = nullptr;
  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;
};
