#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;
class ezWindWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Velocity final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Velocity, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Velocity();
  ~ezParticleBehaviorFactory_Velocity();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  float m_fRiseSpeed = 0;
  float m_fFriction = 0;
  float m_fWindInfluence = 0;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Velocity final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Velocity, ezParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fRiseSpeed = 0;
  float m_fFriction = 0;
  float m_fWindInfluence = 0;

protected:
  friend class ezParticleBehaviorFactory_Velocity;

  virtual void Process(ezUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) override;

  // used to rise/fall along the gravity vector
  ezPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;
};
