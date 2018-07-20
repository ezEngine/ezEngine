#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;
class ezWindWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Velocity : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Velocity, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Velocity();
  ~ezParticleBehaviorFactory_Velocity();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  float m_fRiseSpeed = 0;
  float m_fFriction = 0;
  float m_fWindInfluence = 0;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Velocity : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Velocity, ezParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fRiseSpeed = 0;
  float m_fFriction = 0;
  float m_fWindInfluence = 0;

protected:
  virtual void AfterPropertiesConfigured(bool bFirstTime) override;
  virtual void Process(ezUInt64 uiNumElements) override;

  // used to rise/fall along the gravity vector
  ezPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;
  ezWindWorldModuleInterface* m_pWindModule = nullptr;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;
};

