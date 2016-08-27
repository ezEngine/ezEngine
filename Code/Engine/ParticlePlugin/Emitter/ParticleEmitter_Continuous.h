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

  ezTime m_Duration;

  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;

  ezTime m_SpawnIntervalMin;
  ezTime m_SpawnIntervalRange;

  ezCurve1DResourceHandle m_hCountCurve;
  ezTime m_CurveDuration;


  virtual void CreateRequiredStreams() override;

protected:
  virtual void SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  virtual bool IsFinished() override { return m_Duration < ezTime::Seconds(0); }
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  ezTime m_NextSpawn;
  ezTime m_CountCurveTime;

  //ezStream* m_pStreamPosition;
  //ezStream* m_pStreamVelocity;
  //ezStream* m_pStreamColor;
};
