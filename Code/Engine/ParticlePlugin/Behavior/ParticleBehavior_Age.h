#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Age : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Age, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Age();

  virtual ezParticleBehavior* CreateBehavior(ezParticleSystemInstance* pOwner) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Age : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Age, ezParticleBehavior);

public:
  ezParticleBehavior_Age(ezParticleSystemInstance* pOwner);

protected:
  virtual void Process(ezUInt64 uiNumElements) override;

  virtual void StepParticleSystem() override;

};