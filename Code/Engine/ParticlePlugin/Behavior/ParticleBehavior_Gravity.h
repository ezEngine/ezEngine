#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Gravity : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Gravity, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Gravity();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  float m_fGravityFactor;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Gravity : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Gravity, ezParticleBehavior);

public:
  float m_fGravityFactor;

  virtual void AfterPropertiesConfigured(bool bFirstTime) override;
  virtual void CreateRequiredStreams() override;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  ezPhysicsWorldModuleInterface* m_pPhysicsModule;

  ezProcessingStream* m_pStreamVelocity;
};

