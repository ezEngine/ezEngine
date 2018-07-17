#pragma once

#include <ParticlePlugin/Events/ParticleEventReaction.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleEventReactionFactory_Effect : public ezParticleEventReactionFactory
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReactionFactory_Effect, ezParticleEventReactionFactory);

public:
  ezParticleEventReactionFactory_Effect();

  virtual const ezRTTI* GetEventReactionType() const override;
  virtual void CopyReactionProperties(ezParticleEventReaction* pObject) const override;

  virtual void Save(ezStreamWriter& stream) const override;
  virtual void Load(ezStreamReader& stream) override;

  ezString m_sEffect;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction_Effect : public ezParticleEventReaction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction_Effect, ezParticleEventReaction);

public:
  ezParticleEventReaction_Effect();
  ~ezParticleEventReaction_Effect();

  ezParticleEffectResourceHandle m_hEffect;

protected:
  virtual void ProcessEventQueue(const ezParticleEventQueue* pQueue) override;
};
