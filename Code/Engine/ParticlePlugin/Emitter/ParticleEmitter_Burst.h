#pragma once

#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <Foundation/Types/VarianceTypes.h>

class ezParticleEmitterFactory_Burst : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_Burst, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_Burst();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  ezTime m_Duration;
  ezTime m_StartDelay;

  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;
  ezString m_sSpawnCountScaleParameter;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Burst : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Burst, ezParticleEmitter);

public:

  ezTime m_Duration; // overall duration in which the emitter is considered active, 0 for single frame
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
