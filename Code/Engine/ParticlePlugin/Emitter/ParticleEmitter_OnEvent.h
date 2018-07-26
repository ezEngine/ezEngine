#pragma once

#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <Core/ResourceManager/ResourceHandle.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <Foundation/Containers/Deque.h>

class ezParticleEmitterFactory_OnEvent : public ezParticleEmitterFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitterFactory_OnEvent, ezParticleEmitterFactory);

public:
  ezParticleEmitterFactory_OnEvent();

  virtual const ezRTTI* GetEmitterType() const override;
  virtual void CopyEmitterProperties(ezParticleEmitter* pEmitter, bool bFirstTime) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sEventName;

};


class EZ_PARTICLEPLUGIN_DLL ezParticleEmitter_OnEvent : public ezParticleEmitter
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEmitter_OnEvent, ezParticleEmitter);

public:

  ezTempHashedString m_sEventName;

  virtual void CreateRequiredStreams() override;

protected:
  virtual void InitializeElements(ezUInt64 uiStartIndex, ezUInt64 uiNumElements) override;

  virtual ezParticleEmitterState IsFinished() override;
  virtual ezUInt32 ComputeSpawnCount(const ezTime& tDiff) override;

  virtual void ProcessEventQueue(ezParticleEventQueue queue) override;

  ezProcessingStream* m_pStreamPosition;
  ezProcessingStream* m_pStreamVelocity;

  ezStaticArray<ezParticleEvent, 16> m_Events;
};
