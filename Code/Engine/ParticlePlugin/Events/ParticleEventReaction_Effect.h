#pragma once

#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <GameEngine/Surfaces/SurfaceResourceDescriptor.h>
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
  ezEnum<ezSurfaceInteractionAlignment> m_Alignment;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  ezSharedPtr<ezParticleEffectParameters> m_Parameters;
};

class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction_Effect : public ezParticleEventReaction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction_Effect, ezParticleEventReaction);

public:
  ezParticleEventReaction_Effect();
  ~ezParticleEventReaction_Effect();

  ezParticleEffectResourceHandle m_hEffect;
  ezEnum<ezSurfaceInteractionAlignment> m_Alignment;
  ezSharedPtr<ezParticleEffectParameters> m_Parameters;

protected:
  virtual void ProcessEvent(const ezParticleEvent& e) override;
};
