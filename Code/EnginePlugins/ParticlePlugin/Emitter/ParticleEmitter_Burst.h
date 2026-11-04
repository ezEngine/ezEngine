#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

/// Emitter that spawns a burst of particles over a short duration
///
/// Spawns all particles distributed over the duration time.
/// After the burst completes, the emitter becomes inactive.
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Burst final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Burst, ezParticleEmitter);

public:
  ezParticleEmitter_Burst();

  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual ezParticleEmitterState IsFinished(const ezParticleModuleContext& ctx) const override;
  virtual ezUInt32 ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& diff) const override;

public:
  ezTime m_Duration;                    ///< Duration over which to distribute the burst (0 = single frame)
  ezTime m_StartDelay;                  ///< Delay before burst starts

  ezUInt32 m_uiSpawnCountMin;           ///< Minimum number of particles to spawn
  ezUInt32 m_uiSpawnCountRange;         ///< Random range added to spawn count
  ezHashedString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezTime m_StartDelay; ///< Counts down once, has no effect later on.
    ezUInt32 m_uiSpawnCountLeft;
    float m_fSpawnPerSecond;
    float m_fSpawnAccu;
  };
};
