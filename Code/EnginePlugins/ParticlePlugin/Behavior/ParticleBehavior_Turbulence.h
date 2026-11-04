#pragma once

#include <Foundation/SimdMath/SimdNoise.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>

/// Applies a turbulent force to particles using 3D curl noise.
///
/// Curl noise produces divergence-free flow fields that look natural and avoid
/// compression artifacts. Useful for smoke, fire, and magical effects.
class EZ_PARTICLEPLUGIN_DLL ezParticleBehavior_Turbulence final : public ezParticleBehavior
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleBehavior_Turbulence, ezParticleBehavior);

public:
  ezParticleBehavior_Turbulence();

  virtual void QueryFinalizerDependencies(ezSet<const ezRTTI*>& inout_finalizerDeps) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual void Process(const ezParticleModuleContext& ctx, ezUInt64 uiNumElements) const override;

  float m_fStrength = 2.0f;
  float m_fFrequency = 1.0f;
  ezVec3 m_vScrollSpeed = ezVec3::MakeZero();
  ezUInt8 m_uiOctaves = 1;
  bool m_bAffectVelocity = true;

private:
  struct InstanceData
  {
    ezProcessingStream* m_pStreamPosition = nullptr;
    ezProcessingStream* m_pStreamVelocity = nullptr;

    ezSimdPerlinNoise m_Noise;
    ezTime m_TotalTime;
  };
};
