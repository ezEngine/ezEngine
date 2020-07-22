#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/Deque.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Events/ParticleEvent.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleEmitterFactory_OnEvent final : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_OnEvent, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_OnEvent();
  ~ezParticleEmitterFactory_OnEvent();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter, bool bFirstTime) const override;
  virtual void QueryMaxParticleCount(ezUInt32& out_uiMaxParticlesAbs, ezUInt32& out_uiMaxParticlesPerSecond) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sEventName;
  ezUInt32 m_uiSpawnCountMin = 1;
  ezUInt32 m_uiSpawnCountRange = 0;
  ezString m_sSpawnCountScaleParameter;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_OnEvent final : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_OnEvent, ezParticleEmitter);

public:
  ezTempHashedString m_sEventName;
  ezUInt32 m_uiSpawnCountMin = 1;
  ezUInt32 m_uiSpawnCountRange = 0;
  ezTempHashedString m_sSpawnCountScaleParameter;

  virtual void CreateRequiredStreams() override {}

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override {}

  virtual ezParticleEmitterState IsFinished() override;
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  virtual void ProcessEventQueue(ezParticleEventQueue queue) override;

  bool m_bSpawn = false;
};
