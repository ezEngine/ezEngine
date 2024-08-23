#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>

using ezCurve1DResourceHandle = ezTypedResourceHandle<class ezCurve1DResource>;

class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory_Continuous final : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_Continuous, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_Continuous();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;

public:
  ezTime m_StartDelay;

  ezUInt32 m_uiSpawnCountPerSec;
  ezUInt32 m_uiSpawnCountPerSecRange;
  ezString m_sSpawnCountScaleParameter;

  ezCurve1DResourceHandle m_hCountCurve;
  ezTime m_CurveDuration;
};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Continuous final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_Continuous, ezParticleEmitter);

public:
  ezTime m_StartDelay; // delay before the emitter becomes active, to sync with other systems, only used once, has no effect later on

  ezUInt32 m_uiSpawnCountPerSec;
  ezUInt32 m_uiSpawnCountPerSecRange;
  ezTempHashedString m_sSpawnCountScaleParameter;

  ezCurve1DResourceHandle m_hCountCurve;
  ezTime m_CurveDuration;


  virtual void CreateRequiredStreams() override {}

protected:
  virtual bool IsContinuous() const override { return true; }

  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}
  virtual void OnFinalize() override;

  virtual ezParticleEmitterState IsFinished() override;
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  ezTime m_CountCurveTime;
  ezTime m_TimeSinceRandom;
  float m_fCurSpawnPerSec;
  float m_fCurSpawnCounter;
};
