#pragma once

#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <Core/ResourceManager/ResourceHandle.h>

typedef ezTypedResourceHandle<class ezCurve1DResource> ezCurve1DResourceHandle;

class ezParticleEmitterFactory_Continuous : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_Continuous, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_Continuous();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  ezTime m_Duration;
  ezTime m_StartDelay;

  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;

  ezTime m_SpawnIntervalMin;
  ezTime m_SpawnIntervalRange;

  ezCurve1DResourceHandle m_hCountCurve;
  ezTime m_CurveDuration;

  void SetCountCurveFile(const char* szFile);
  const char* GetCountCurveFile() const;

};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Continuous : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Continuous, ezParticleEmitter);

public:

  ezTime m_Duration; // overall duration in which the emitter is considered active, 0 for endless, -1 for finished
  ezTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;

  ezTime m_SpawnIntervalMin;
  ezTime m_SpawnIntervalRange;

  ezCurve1DResourceHandle m_hCountCurve;
  ezTime m_CurveDuration;


  virtual void CreateRequiredStreams() override {}
  virtual void AfterPropertiesConfigured(bool bFirstTime) override;

protected:
  virtual void SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}

  virtual ezParticleEmitterState IsFinished() override;
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  ezTime m_RunningTime;
  ezTime m_NextSpawn;
  ezTime m_CountCurveTime;
};
