#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezWindWorldModuleInterface;

/// Behavior that applies wind forces to particles
///
/// Reads wind samples from the world and applies them to particle positions.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Wind final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Wind, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Wind();
  ~ezParticleBehaviorFactory_Wind();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fWindInfluence = 1.0f;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Wind final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Wind, ezParticleBehavior);

public:
  virtual void CreateRequiredStreams() override;

  float m_fWindInfluence = 1.0f;

protected:
  friend class ezParticleBehaviorFactory_Wind;

  virtual void Process(ezUInt64 uiNumElements) override;

  void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
};
