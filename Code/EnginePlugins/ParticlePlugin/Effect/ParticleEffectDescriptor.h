#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Declarations.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class EZ_PARTICLEPLUGIN_DLL ezParticleEffectDescriptor final : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEffectDescriptor, ezReflectedClass);

public:
  ezParticleEffectDescriptor();
  ~ezParticleEffectDescriptor();

  void AddParticleSystem(ezParticleSystemDescriptor* pSystem) { m_ParticleSystems.PushBack(pSystem); }
  void RemoveParticleSystem(ezParticleSystemDescriptor* pSystem) { m_ParticleSystems.RemoveAndCopy(pSystem); }
  const ezHybridArray<ezParticleSystemDescriptor*, 4>& GetParticleSystems() const { return m_ParticleSystems; }

  void AddEventReaction(ezParticleEventReaction* pSystem) { m_EventReactions.PushBack(pSystem); }
  void RemoveEventReaction(ezParticleEventReaction* pSystem) { m_EventReactions.RemoveAndCopy(pSystem); }
  const ezHybridArray<ezParticleEventReaction*, 4>& GetEventReactions() const { return m_EventReactions; }


  /// Finds a module type by the name stored in a transformed effect.
  ///
  /// Effects that were transformed before the module classes got renamed still contain the old names, which contained 'Factory'.
  /// Returns nullptr for unknown types.
  static const ezRTTI* FindModuleType(ezStringView sTypeName);

  void Save(ezStreamWriter& inout_stream) const;
  void Load(ezStreamReader& inout_stream);

  void ClearSystems();
  void ClearEventReactions();

  ezEnum<ezEffectInvisibleUpdateRate> m_InvisibleUpdateRate;
  bool m_bSimulateInLocalSpace = false;
  bool m_bAlwaysShared = false;
  float m_fApplyInstanceVelocity = 0.0f;
  ezTime m_PreSimulateDuration;
  ezVec3U32 m_vNumWindSamples = ezVec3U32(1);
  ezMap<ezString, float> m_FloatParameters;
  ezMap<ezString, ezColor> m_ColorParameters;

private:
  ezHybridArray<ezParticleSystemDescriptor*, 4> m_ParticleSystems;
  ezHybridArray<ezParticleEventReaction*, 4> m_EventReactions;
};
