#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Basics.h>
#include <ParticlePlugin/Declarations.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectDescriptor : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectDescriptor, ezReflectedClass);

public:
  ezParticleEffectDescriptor();
  ~ezParticleEffectDescriptor();

  void AddParticleSystem(ezParticleSystemDescriptor* pSystem) { m_ParticleSystems.PushBack(pSystem); }
  void RemoveParticleSystem(ezParticleSystemDescriptor* pSystem) { m_ParticleSystems.Remove(pSystem); }
  const ezHybridArray<ezParticleSystemDescriptor*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddEventReaction(ezParticleEventReactionFactory* pSystem) { m_EventReactions.PushBack(pSystem); }
  void RemoveEventReaction(ezParticleEventReactionFactory* pSystem) { m_EventReactions.Remove(pSystem); }
  const ezHybridArray<ezParticleEventReactionFactory*, 4>& GetEventReactions() const { return m_EventReactions; }


  void Save(ezStreamWriter& stream) const;
  void Load(ezStreamReader& stream);

  void ClearSystems();
  void ClearEventReactions();

  ezEnum<ezEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  bool m_bSimulateInLocalSpace = false;
  bool m_bAlwaysShared = false;
  float m_fApplyInstanceVelocity = 0.0f;
  ezTime m_PreSimulateDuration;
  ezMap<ezString, float> m_FloatParameters;
  ezMap<ezString, ezColor> m_ColorParameters;

private:
  ezHybridArray<ezParticleSystemDescriptor*, 4> m_ParticleSystems;
  ezHybridArray<ezParticleEventReactionFactory*, 4> m_EventReactions;
};
