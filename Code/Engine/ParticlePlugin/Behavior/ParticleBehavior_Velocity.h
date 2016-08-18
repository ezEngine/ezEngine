#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Velocity : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Velocity, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Velocity();

  virtual ezParticleBehavior* CreateBehavior(ezParticleSystemInstance* pOwner) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  //float m_fVelocityDamping;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Velocity : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Velocity, ezParticleBehavior);

public:
  ezParticleBehavior_Velocity(ezParticleSystemInstance* pOwner);

  //float m_fVelocityDamping;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;
  virtual void StepParticleSystem() override;
};

