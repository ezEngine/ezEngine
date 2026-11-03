#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

/// Emitter that spawns particles based on distance traveled
///
/// Spawns particles when the effect moves a certain distance from the last spawn position.
/// Useful for creating trails or footstep effects.
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Distance final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Distance, ezParticleEmitter);

public:
  ezParticleEmitter_Distance();

  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual bool IsContinuous() const override { return true; }
  virtual ezParticleEmitterState IsFinished(const ezParticleModuleContext& ctx) const override;
  virtual ezUInt32 ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& tDiff) const override;

public:
  float m_fDistanceThreshold = 0.1f;    ///< Distance that must be traveled before spawning
  ezUInt32 m_uiSpawnCountMin = 1;       ///< Minimum particles per spawn
  ezUInt32 m_uiSpawnCountRange = 0;     ///< Random range added to spawn count
  ezHashedString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    bool m_bFirstUpdate;
    ezVec3 m_vLastSpawnPosition;
  };
};
