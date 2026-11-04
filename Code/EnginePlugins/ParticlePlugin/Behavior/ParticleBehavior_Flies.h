#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Behavior that makes particles move like flies or insects
///
/// Particles move at a constant speed with random direction changes.
/// They tend to stay within a certain distance from the effect origin.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Flies final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Flies, ezParticleBehavior);

public:
  ezParticleBehavior_Flies();
  ~ezParticleBehavior_Flies();


  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  float m_fSpeed = 0.2f;                                    ///< Movement speed in units per second
  float m_fPathLength = 0.2f;                               ///< Distance traveled before changing direction
  float m_fMaxEmitterDistance = 0.5f;                       ///< Maximum distance from effect origin before turning back
  ezAngle m_MaxSteeringAngle = ezAngle::MakeFromDegree(30); ///< Maximum angle change per direction update

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezProcessingStream* m_pStreamPosition;
    ezProcessingStream* m_pStreamVelocity;
    ezTime m_TimeToChangeDir;
  };
};
