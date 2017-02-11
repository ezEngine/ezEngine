#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Util/ParticleUtils.h>

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

  virtual void AfterPropertiesConfigured(bool bFirstTime) override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);

  bool m_bHasOnDeathEventHandler;
  ezProcessingStream* m_pStreamLifeTime;
  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;
};
