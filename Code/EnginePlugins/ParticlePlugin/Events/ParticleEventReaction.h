#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleEffectInstance;

/// Base class for all particle event reactions
///
/// Event reactions respond to particle events by performing actions like spawning effects or prefabs.
/// They are shared between all instances of an effect and have no state per instance.
/// Each reaction specifies which event type to respond to and the probability of triggering.
class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction, ezReflectedClass);

public:
  virtual void Save(ezStreamWriter& inout_stream) const;
  virtual void Load(ezStreamReader& inout_stream);

  /// Called when a matching event occurs and the probability check passes.
  ///
  /// Derived classes implement the actual reaction behavior (e.g., spawning an effect).
  virtual void ProcessEvent(ezParticleEffectInstance* pOwner, const ezParticleEvent& e) const = 0;

  ezHashedString m_sEventType; ///< The type of particle event this reaction responds to

  /// Probability (1-100) that this reaction triggers when the event occurs.
  ///
  /// At most one reaction triggers per event. All reactions of an effect that respond to the same event type share a single random roll,
  /// so each one gets its own part of the 0-100 range, in the order of the list.
  /// If the probabilities add up to more than 100, the reactions further down the list get less than their value, or never trigger.
  ezUInt8 m_uiProbability = 100;
};
