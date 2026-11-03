#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

using ezCurve1DResourceHandle = ezTypedResourceHandle<class ezCurve1DResource>;

/// Emitter that continuously spawns particles over time
///
/// Spawn rate can be constant or modulated by a curve.
/// Continues emitting until the effect is stopped.
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Continuous final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Continuous, ezParticleEmitter);

public:
  ezParticleEmitter_Continuous();

  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;
  virtual void OnFinalize(const ezParticleModuleContext& ctx) const override;
  virtual bool IsContinuous() const override { return true; }
  virtual ezParticleEmitterState IsFinished(const ezParticleModuleContext& ctx) const override;
  virtual ezUInt32 ComputeSpawnCount(const ezParticleModuleContext& ctx, const ezTime& tDiff) const override;

public:
  ezTime m_StartDelay;                   ///< Delay before emission starts

  ezUInt32 m_uiSpawnCountPerSec;         ///< Base spawn rate per second
  ezUInt32 m_uiSpawnCountPerSecRange;    ///< Random range added to spawn rate
  ezHashedString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn rate

  ezCurve1DResourceHandle m_hCountCurve; ///< Optional curve to modulate spawn rate
  ezTime m_CurveDuration;                ///< Duration for curve evaluation

private:
  struct InstanceData
  {
    EZ_DECLARE_POD_TYPE();

    ezTime m_StartDelay; ///< Counts down once, has no effect later on.
    ezTime m_CountCurveTime;
    ezTime m_TimeSinceRandom;
    float m_fCurSpawnPerSec;
    float m_fCurSpawnCounter;
  };

  float ComputeRandomSpawnPerSec(const ezParticleModuleContext& ctx) const;
};
