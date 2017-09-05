#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_PullAlong : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_PullAlong, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_PullAlong();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  float m_fStrength;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_PullAlong : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_PullAlong, ezParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fStrength = 0.5;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;
  virtual void StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles) override;

  bool m_bFirstTime = true;
  ezUInt32 m_uiIgnoreNewParticles = 0;
  ezVec3 m_vLastEmitterPosition;
  ezVec3 m_vApplyPull;
  ezProcessingStream* m_pStreamPosition;
};

