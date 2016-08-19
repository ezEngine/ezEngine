#pragma once

#include <ParticlePlugin/Emitter/ParticleEmitter.h>

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
  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;

  ezTime m_SpawnIntervalMin;
  ezTime m_SpawnIntervalRange;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Continuous : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Continuous, ezParticleEmitter);

public:

  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;

  ezTime m_SpawnIntervalMin;
  ezTime m_SpawnIntervalRange;

protected:
  virtual void SpawnElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  ezTime m_NextSpawn;
};
