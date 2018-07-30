#pragma once

#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Types/VarianceTypes.h>

typedef ezTypedResourceHandle<class ezCurve1DResource> ezCurve1DResourceHandle;

class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory_Continuous : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_Continuous, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_Continuous();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

public:
  ezTime m_StartDelay;

  ezUInt32 m_uiSpawnCountPerSec;
  ezUInt32 m_uiSpawnCountPerSecRange;
  ezString m_sSpawnCountScaleParameter;

  ezCurve1DResourceHandle m_hCountCurve;
  ezTime m_CurveDuration;

  void SetCountCurveFile(const char* szFile);
  const char* GetCountCurveFile() const;

};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_Continuous : public ezParticleEmitter
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
