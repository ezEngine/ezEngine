#pragma once

#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Pulls particles toward a point with distance-based falloff.
///
/// Can modify velocity (physically correct acceleration) or position directly (snapping).
/// MinDistance prevents singularity at the attractor center.
///
/// The behavior queries the spatial system for nearby ezParticleAttractorComponents and applies each one's contribution per particle.
/// The attractor list is refreshed roughly once per second for performance, the attractor positions and properties are read every frame.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehaviorFactory_Attract final : public ezParticleBehaviorFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehaviorFactory_Attract, ezParticleBehaviorFactory);

public:
  ezParticleBehaviorFactory_Attract();

  virtual const ezRTTI* GetBehaviorType() const override;
  virtual void CopyBehaviorProperties(ezParticleBehavior* pObject, bool bFirstTime) const override;
  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  float m_fInfluence = 5.0f;
  bool m_bAffectVelocity = true;

  /// Maximum number of nearby attractors to consider. Higher values are more expensive.
  ezUInt8 m_uiMaxAttractors = 1;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Attract final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Attract, ezParticleBehavior);

public:
  float m_fInfluence = 5.0f;
  bool m_bAffectVelocity = true;
  ezUInt8 m_uiMaxAttractors = 1;

  virtual void OnFinalize() override;
  virtual void UpdateFromWorld(const ezWorld& world, ezGameObjectHandle hOwnerObject) override;

protected:
  virtual void CreateRequiredStreams() override;
  virtual void Process(ezUInt64 uiNumElements) override;

  ezProcessingStream* m_pStreamPosition = nullptr;
  ezProcessingStream* m_pStreamVelocity = nullptr;

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

  ezUInt8 m_uiAttractorSearchTimer = 0;
  ezSmallArray<ezComponentHandle, 1> m_AttractorHandles;
  ezSmallArray<AttractorData, 1> m_Attractors;
};
