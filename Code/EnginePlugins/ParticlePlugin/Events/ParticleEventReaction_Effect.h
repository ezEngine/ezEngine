#pragma once

#include <Core/Physics/SurfaceResourceDescriptor.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/SharedPtr.h>
#include <ParticlePlugin/Events/ParticleEventReaction.h>

/// Reaction that spawns a particle effect at the location of the event.
class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction_Effect final : public ezParticleEventReaction
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction_Effect, ezParticleEventReaction);

public:
  ezParticleEventReaction_Effect();

  virtual void Save(ezStreamWriter& inout_stream) const override;
  virtual void Load(ezStreamReader& inout_stream) override;
  virtual void ProcessEvent(ezParticleEffectInstance* pOwner, const ezParticleEvent& e) const override;

  ezString m_sEffect;
  ezEnum<ezSurfaceInteractionAlignment> m_Alignment;

  ezParticleEffectResourceHandle m_hEffect;

  //////////////////////////////////////////////////////////////////////////
  // Exposed Parameters
public:
  const ezRangeView<const char*, ezUInt32> GetParameters() const;
  void SetParameter(const char* szKey, const ezVariant& value);
  void RemoveParameter(const char* szKey);
  bool GetParameter(const char* szKey, ezVariant& out_value) const;

private:
  ezSharedPtr<ezParticleEffectParameters> m_pParameters;
};
