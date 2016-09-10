#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Age : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Age, ezParticleBehaviorFactory);

public:
  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sOnDeathEvent;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Age : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Age, ezParticleBehavior);
public:
  ezParticleBehavior_Age();
  ~ezParticleBehavior_Age();

  virtual void CreateRequiredStreams() override;

  ezTempHashedString m_sOnDeathEvent;

  virtual void AfterPropertiesConfigured(bool bFirstTime) override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;
  void OnParticleDeath(const ezStreamGroupElementRemovedEvent& e);

  bool m_bHasOnDeathEventHandler;
  ezStream* m_pStreamLifeTime;
  ezStream* m_pStreamPosition;
  ezStream* m_pStreamVelocity;
};