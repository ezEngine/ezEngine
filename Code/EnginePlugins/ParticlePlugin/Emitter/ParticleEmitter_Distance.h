#pragma once

#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory_Distance final : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_Distance, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_Distance();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

public:
  float m_fDistanceThreshold = 0.1f;
  ezUInt32 m_uiSpawnCountMin = 1;
  ezUInt32 m_uiSpawnCountRange = 0;
  ezString m_sSpawnCountScaleParameter;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Distance final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Distance, ezParticleEmitter);

public:
  float m_fDistanceThresholdSQR;
  ezUInt32 m_uiSpawnCountMin;
  ezUInt32 m_uiSpawnCountRange;
  ezTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override;

protected:
  virtual bool IsContinuous() const override;
  virtual void OnFinalize() override;
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  virtual ezParticleEmitterState IsFinished() override;
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  bool m_bFirstUpdate = true;
  ezVec3 m_vLastSpawnPosition;
};
