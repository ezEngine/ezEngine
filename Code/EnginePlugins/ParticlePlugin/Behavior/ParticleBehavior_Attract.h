#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Pulls particles toward a point with distance-based falloff.
///
/// Can modify velocity (physically correct acceleration) or position directly (snapping).
/// MinDistance prevents singularity at the attractor center.
///
/// The behavior queries the spatial system for nearby ezParticleAttractorComponents and applies each one's contribution per particle.
/// The attractor list is refreshed roughly once per second for performance, the attractor positions and properties are read every frame.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Attract final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Attract, ezParticleBehavior);

public:
  ezParticleBehavior_Attract();

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual bool GetUpdateFromWorld() const override { return true; }
  virtual void UpdateFromWorld(const ezParticleModuleContext& ctx, const ezWorld& world, ezGameObjectHandle hOwnerObject) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  float m_fInfluence = 5.0f;
  bool m_bAffectVelocity = true;

  /// Maximum number of nearby attractors to consider. Higher values are more expensive.
  ezUInt8 m_uiMaxAttractors = 1;

private:
  /// Data of one attractor, written in UpdateFromWorld() and read during the particle update.
  struct AttractorData
  {
    EZ_DECLARE_POD_TYPE();

    ezVec3 m_vPosition;
    float m_fStrength;
    float m_fRadius;
    float m_fMinDistance;
    float m_fKillDistance;
  };

  struct InstanceData
  {
    ezProcessingStream* m_pStreamPosition = nullptr;
    ezProcessingStream* m_pStreamVelocity = nullptr;

    ezUInt8 m_uiAttractorSearchTimer = 0;
    ezSmallArray<ezComponentHandle, 1> m_AttractorHandles;
    ezSmallArray<AttractorData, 1> m_Attractors;
  };
};
