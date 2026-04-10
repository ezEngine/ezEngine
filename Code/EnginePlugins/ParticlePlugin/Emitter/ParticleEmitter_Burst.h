#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

/// Emitter that spawns a burst of particles over a short duration
///
/// Spawns all particles distributed over the duration time.
/// After the burst completes, the emitter becomes inactive.
class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory_Burst final : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_Burst, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_Burst();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream, const ezParticleEffectDescriptor& ownerEffectDescriptor, const ezParticleSystemDescriptor& ownerSystemDescriptor) override;

public:
  ezTime m_Duration;                    ///< Duration over which to distribute the burst (0 = single frame)
  ezTime m_StartDelay;                  ///< Delay before burst starts

  ezUInt32 m_uiSpawnCountMin;           ///< Minimum number of particles to spawn
  ezUInt32 m_uiSpawnCountRange;         ///< Random range added to spawn count
  ezString m_sSpawnCountScaleParameter; ///< Optional parameter to scale spawn count
};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Burst final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Burst, ezParticleEmitter);

public:
  ezTime m_Duration;   // overall duration in which the emitter is considered active, 0 for single frame
  ezTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;
  ezTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void OnFinalize() override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}

  virtual ezParticleEmitterState IsFinished() override;
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  ezUInt32 m_uiSpawnCountLeft = 0;
  float m_fSpawnPerSecond = 0;
  float m_fSpawnAccu = 0;
};
