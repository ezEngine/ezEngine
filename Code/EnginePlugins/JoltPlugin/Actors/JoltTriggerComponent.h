#pragma once

#include <Core/Messages/TriggerMessage.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

//////////////////////////////////////////////////////////////////////////

class EZ_JOLTPLUGIN_DLL ezJoltTriggerComponentManager : public ezComponentManager<class ezJoltTriggerComponent, ezBlockStorageType::FreeList>
{
public:
  ezJoltTriggerComponentManager(ezWorld* pWorld);
  ~ezJoltTriggerComponentManager();

private:
  friend class ezJoltWorldModule;
  friend class ezJoltTriggerComponent;

  void UpdateMovingTriggers();

  ezSet<ezJoltTriggerComponent*> m_MovingTriggers;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Turns an object into a trigger that is capable of detecting when other physics objects enter its volume.
///
/// Triggers are physics actors and thus are set up the same way, e.g. they use physics shapes for their geometry,
/// but they act very differently. Triggers do not affect other objects, instead all objects just pass through them.
/// However, the trigger detects overlap with other objects and sends a message when a new object enters its volume
/// or when one leaves it.
///
/// \note The physics trigger only sends enter and leave messages. It does not send any message when an object stays inside
/// the trigger.
///
/// The message ezMsgTriggerTriggered is sent for every change. It references the object that entered or left the volume
/// and it also contains a trigger-specific message string to identify what this should be used for.
class EZ_JOLTPLUGIN_DLL ezJoltTriggerComponent : public ezJoltActorComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezJoltTriggerComponent, ezJoltActorComponent, ezJoltTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // ezJoltTriggerComponent
public:
  ezJoltTriggerComponent();
  ~ezJoltTriggerComponent();

  /// \brief Sets the text that the ezMsgTriggerTriggered should contain when the trigger fires.
  void SetTriggerMessage(const char* szSz) { m_sTriggerMessage.Assign(szSz); }  // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]

protected:
  friend class ezJoltWorldModule;
  friend class ezJoltContactListener;

  void PostTriggerMessage(const ezGameObjectHandle& hOtherObject, ezTriggerState::Enum triggerState) const;

  ezHashedString m_sTriggerMessage;
  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
