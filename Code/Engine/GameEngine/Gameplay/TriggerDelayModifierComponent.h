#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

struct ezMsgTriggerTriggered;
struct ezMsgComponentInternalTrigger;

using ezTriggerDelayModifierComponentManager = ezComponentManager<class ezTriggerDelayModifierComponent, ezBlockStorageType::Compact>;

/// \brief Handles ezMsgTriggerTriggered events and sends new messages after a delay.
///
/// The 'enter' and 'leave' messages are sent only when an empty trigger is entered or when the last object leaves the trigger.
/// While any object is already inside the trigger, no change event is sent.
/// Therefore this component can't be used to keep track of all the objects inside the trigger.
///
/// The 'enter' and 'leave' events can be sent with a delay. The 'enter' event is only sent, if the trigger had at least one object
/// inside it for the full duration of the delay. Which exact object may change, but once the trigger contains no object, the timer is reset.
///
/// The sent ezMsgTriggerTriggered does not contain a reference to the 'triggering' object, since there may be multiple and they may change randomly.
class EZ_GAMEENGINE_DLL ezTriggerDelayModifierComponent : public ezEventMessageHandlerBaseComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezTriggerDelayModifierComponent, ezEventMessageHandlerBaseComponent, ezTriggerDelayModifierComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezTriggerDelayModifierComponent

public:
  ezTriggerDelayModifierComponent();
  ~ezTriggerDelayModifierComponent();

protected:
  virtual void Initialize() override;

  void OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg);
  void OnMsgComponentInternalTrigger(ezMsgComponentInternalTrigger& msg);

  bool m_bIsActivated = false;
  ezInt32 m_iElementsInside = 0;
  ezInt32 m_iValidActivationToken = 0;
  ezInt32 m_iValidDeactivationToken = 0;

  ezTime m_ActivationDelay;
  ezTime m_DeactivationDelay;
  ezHashedString m_sMessage;

  ezEventMessageSender<ezMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
