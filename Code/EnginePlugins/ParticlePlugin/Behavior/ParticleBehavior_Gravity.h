#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

class ezPhysicsWorldModuleInterface;

/// Behavior that applies gravity to particles
///
/// Uses the world's gravity vector from the physics module.
/// The gravity factor scales the applied gravity force.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Gravity final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Gravity, ezParticleBehavior);

public:
  ezParticleBehavior_Gravity();


  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;
  virtual void RequestRequiredWorldModulesForCache(ezParticleWorldModule* pParticleModule) const override;

public:
  float m_fGravityFactor;

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezPhysicsWorldModuleInterface* m_pPhysicsModule;
    ezProcessingStream* m_pStreamVelocity;
  };
};
