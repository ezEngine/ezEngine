#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <Foundation/Types/VarianceTypes.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Age : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Age, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Age();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezVarianceTypeTime m_LifeTime;
  ezString m_sOnDeathEvent;
  ezString m_sLifeScaleParameter;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Age : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Age, ezParticleBehavior);
public:
  ezParticleBehavior_Age();
  ~ezParticleBehavior_Age();

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
