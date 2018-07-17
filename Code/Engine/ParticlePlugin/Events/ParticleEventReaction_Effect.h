#pragma once

#include <ParticlePlugin/Events/ParticleEventReaction.h>
#include <Foundation/Types/RangeView.h>

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

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:

  ezHybridArray<ezParticleEffectFloatParam, 2> m_FloatParams;
  ezHybridArray<ezParticleEffectColorParam, 2> m_ColorParams;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction_Effect : public ezParticleEventReaction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction_Effect, ezParticleEventReaction);

public:
  ezParticleEventReaction_Effect();
  ~ezParticleEventReaction_Effect();

  ezParticleEffectResourceHandle m_hEffect;

  // TODO: maybe could make this just pointers (assuming the factory outlives the instance)
  ezHybridArray<ezParticleEffectFloatParam, 2> m_FloatParams;
  ezHybridArray<ezParticleEffectColorParam, 2> m_ColorParams;

protected:
  virtual void ProcessEventQueue(const ezParticleEventQueue* pQueue) override;
};
