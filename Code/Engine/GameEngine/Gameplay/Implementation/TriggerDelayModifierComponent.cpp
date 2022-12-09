#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/World/GameObject.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/TriggerDelayModifierComponent.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezTriggerDelayModifierComponent, 1 /* version */, ezComponentMode::Static)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ActivationDelay", m_ActivationDelay),
    EZ_MEMBER_PROPERTY("DeactivationDelay", m_DeactivationDelay),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGEHANDLERS
  {
    EZ_MESSAGE_HANDLER(ezMsgComponentInternalTrigger, OnMsgComponentInternalTrigger),
    EZ_MESSAGE_HANDLER(ezMsgTriggerTriggered, OnMsgTriggerTriggered),
  }
  EZ_END_MESSAGEHANDLERS;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Gameplay/Logic"), // Component menu group
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezTriggerDelayModifierComponent::ezTriggerDelayModifierComponent() = default;
ezTriggerDelayModifierComponent::~ezTriggerDelayModifierComponent() = default;

void ezTriggerDelayModifierComponent::Initialize()
{
  SUPER::Initialize();
}

void ezTriggerDelayModifierComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_ActivationDelay;
  s << m_DeactivationDelay;
}

void ezTriggerDelayModifierComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_ActivationDelay;
  s >> m_DeactivationDelay;
}

void ezTriggerDelayModifierComponent::OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg)
{
  if (msg.m_TriggerState == ezTriggerState::Activated)
  {
    if (m_iElementsInside++ == 0) // was 0 before the increment
    {
      // the first object entered the trigger

      if (!m_bIsActivated)
      {
        // the trigger is not active yet -> send an activation message with a new activation token

        ++m_iValidActivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        ezMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Activate");
        intMsg.m_iPayload = m_iValidActivationToken;

        PostMessage(intMsg, m_ActivationDelay, ezObjectMsgQueueType::PostTransform);
      }
      else
      {
        // the trigger is already active -> there are pending deactivations (otherwise we wouldn't have had an element count of zero)
        // -> invalidate those pending deactivations
        ++m_iValidDeactivationToken;

        // no need to send an activation message
      }
    }

    return;
  }

  if (msg.m_TriggerState == ezTriggerState::Deactivated)
  {
    if (--m_iElementsInside == 0) // 0 after the decrement
    {
      // the last object left the trigger

      if (m_bIsActivated)
      {
        // if the trigger is active, we need to send a deactivation message and we give it a new token

        ++m_iValidDeactivationToken;

        // store the original trigger message for later
        m_sMessage = msg.m_sMessage;

        ezMsgComponentInternalTrigger intMsg;
        intMsg.m_sMessage.Assign("Deactivate");
        intMsg.m_iPayload = m_iValidDeactivationToken;

        PostMessage(intMsg, m_DeactivationDelay, ezObjectMsgQueueType::PostTransform);
      }
      else
      {
        // when we are already inactive, all that's needed is to invalidate any pending activations
        ++m_iValidActivationToken;
      }
    }

    return;
  }
}

void ezTriggerDelayModifierComponent::OnMsgComponentInternalTrigger(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == ezTempHashedString("Activate"))
  {
    if (msg.m_iPayload == m_iValidActivationToken && !m_bIsActivated)
    {
      m_bIsActivated = true;

      ezMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = ezTriggerState::Activated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), ezTime::Zero(), ezObjectMsgQueueType::PostTransform);
    }
  }
  else if (msg.m_sMessage == ezTempHashedString("Deactivate"))
  {
    if (msg.m_iPayload == m_iValidDeactivationToken && m_bIsActivated)
    {
      m_bIsActivated = false;

      ezMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = ezTriggerState::Deactivated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), ezTime::Zero(), ezObjectMsgQueueType::PostTransform);
    }
  }
}
