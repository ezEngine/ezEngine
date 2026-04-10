#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

/// Behavior that pulls particles along when the effect moves
///
/// Useful for effects attached to moving objects where particles should
/// follow the movement to some degree rather than staying in world space.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_PullAlong final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_PullAlong, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_PullAlong();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fStrength; ///< How much particles follow the effect movement (0-1 range)
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_PullAlong final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_PullAlong, ezParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fStrength = 0.5;

protected:
  virtual void Process(ezUInt64 uiNumElements) override;
  virtual void StepParticleSystem(const ezTime& tDiff, ezUInt32 uiNumNewParticles) override;

  bool m_bFirstTime = true;
  ezVec3 m_vLastEmitterPosition;
  ezVec3 m_vApplyPull;
  ezProcessingStream* m_pStreamPosition;
};
