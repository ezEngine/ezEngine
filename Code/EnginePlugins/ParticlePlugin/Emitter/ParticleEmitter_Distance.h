#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

/// Emitter that spawns particles based on distance traveled
///
/// Spawns one batch of particles for every 'DistanceThreshold' that the effect moves. When the effect moves farther than that
/// within one update, it spawns several batches and places them along the path at exact intervals, so the spacing does not
/// depend on the frame rate or on how fast the effect moves. At most 64 particles are spawned per update.
///
/// The batches are placed by moving the particles after the initializers ran, thus offsets from position initializers are kept.
/// With local space simulation the particles are not moved, as they move with the effect anyway.
///
/// How many particles spawn per second depends on how fast the effect moves, which is not known up front. Set 'MaxParticles' for fast
/// effects, otherwise the system may be full and drop new particles.
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
  virtual ezUInt32 ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& diff) const override;
  virtual void CreateRequiredStreams(const ezParticleModuleContext& ctx) const override;
  virtual void InitializeElements(const ezParticleModuleContext& ctx, ezUInt64 uiStartIndex, ezUInt64 uiNumElements) const override;

public:
  float m_fDistanceThreshold = 0.1f;    ///< Distance that must be traveled before spawning
  ezUInt32 m_uiSpawnCountMin = 1;       ///< Minimum particles per spawn
  ezUInt32 m_uiSpawnCountRange = 0;     ///< Random range added to spawn count
  ezHashedString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count
  ezUInt32 m_uiMaxParticles = 0;        ///< How many particles of this system can be alive at the same time. 0 estimates it.

private:
  static constexpr ezUInt32 MaxSpawnPerUpdate = 64;

  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    bool m_bFirstUpdate;
    ezVec3 m_vLastSpawnPosition;  ///< Where the last batch was placed.

    // what ComputeSpawnCount() decided, for InitializeElements() of the same update
    ezVec3 m_vEffectPosition;     ///< Where the initializers place the new particles.
    ezVec3 m_vFirstBatchPosition; ///< Where the first new batch goes.
    ezVec3 m_vBatchStep;          ///< The distance from one batch to the next.
    ezUInt32 m_uiNumSpawned;
    ezUInt8 m_uiBatchOfParticle[MaxSpawnPerUpdate];

    ezProcessingStream* m_pStreamPosition;
  };
};
