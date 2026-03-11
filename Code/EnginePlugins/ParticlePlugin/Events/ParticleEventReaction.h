#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class ezParticleEffectInstance;
class ezParticleEventReaction;

/// \brief Base class for all particle event reaction factories
///
/// Event reaction factories create and configure event reactions that respond to particle events.
/// Each factory specifies which event type to respond to and the probability of triggering.
class EZ_PARTICLEPLUGIN_DLL ezParticleEventReactionFactory : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReactionFactory, ezReflectedClass);

public:
  virtual const ezRTTI* GetEventReactionType() const = 0;
  virtual void CopyReactionProperties(ezParticleEventReaction* pObject, bool bFirstTime) const = 0;

  /// Creates and initializes a new event reaction instance for the given effect.
  ezParticleEventReaction* CreateEventReaction(ezParticleEffectInstance* pOwner) const;

  virtual void Save(ezStreamWriter& inout_stream) const;
  virtual void Load(ezStreamReader& inout_stream);

  ezString m_sEventType;         ///< The type of particle event this reaction responds to
  ezUInt8 m_uiProbability = 100; ///< Probability (1-100) that this reaction triggers when the event occurs
};

/// Base class for all particle event reactions.
///
/// Event reactions respond to particle events by performing actions like spawning effects or prefabs.
/// Reactions are checked probabilistically - they may not trigger every time an event occurs.
class EZ_PARTICLEPLUGIN_DLL ezParticleEventReaction : public ezReflectedClass
{
  EZ_ADD_DYNAMIC_REFLECTION(ezParticleEventReaction, ezReflectedClass);

  friend class ezParticleEventReactionFactory;
  friend class ezParticleEffectInstance;

protected:
  ezParticleEventReaction();
  ~ezParticleEventReaction();

  /// Initializes the reaction with a reference to the owning effect instance.
  void Reset(ezParticleEffectInstance* pOwner);

  /// Called when a matching event occurs and the probability check passes.
  ///
  /// Derived classes implement the actual reaction behavior (e.g., spawning an effect).
  virtual void ProcessEvent(const ezParticleEvent& e) = 0;

  ezTempHashedString m_sEventName;                    ///< Hashed event type name for fast comparison
  ezUInt8 m_uiProbability;                            ///< Probability value (1-100) for triggering this reaction
  ezParticleEffectInstance* m_pOwnerEffect = nullptr; ///< The effect instance that owns this reaction
};
