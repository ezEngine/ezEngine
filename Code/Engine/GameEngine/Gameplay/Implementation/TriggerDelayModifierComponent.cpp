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

  SetPassThroughUnhandledEvents(true);
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

void ezTriggerDelayModifierComponent::OnMsgTriggerTriggered(ezMsgTriggerTriggered& msg) const
{
  if (msg.m_TriggerState == ezTriggerState::Activated)
  {
    if (m_iElementsInside.PostIncrement() == 0)
    {
      m_sMessage = msg.m_sMessage;

      ezMsgComponentInternalTrigger intMsg;
      intMsg.m_sMessage.Assign("Activate");

      GetOwner()->GetWorld()->PostMessage(GetHandle(), intMsg, m_ActivationDelay, ezObjectMsgQueueType::PostTransform);
    }
  }
  else if (msg.m_TriggerState == ezTriggerState::Deactivated)
  {
    if (m_iElementsInside.Decrement() == 0)
    {
      m_sMessage = msg.m_sMessage;

      ezMsgComponentInternalTrigger intMsg;
      intMsg.m_sMessage.Assign("Deactivate");

      GetOwner()->GetWorld()->PostMessage(GetHandle(), intMsg, m_DeactivationDelay, ezObjectMsgQueueType::PostTransform);
    }
  }
}

void ezTriggerDelayModifierComponent::OnMsgComponentInternalTrigger(ezMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage.GetString() == "Activate")
  {
    if (m_iElementsInside > 0 && !m_bIsActivated.Set(true))
    {
      ezMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = ezTriggerState::Activated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), ezTime::Zero(), ezObjectMsgQueueType::PostTransform);
    }
  }
  else if (msg.m_sMessage.GetString() == "Deactivate")
  {
    if (m_iElementsInside == 0 && m_bIsActivated.Set(false))
    {
      ezMsgTriggerTriggered newMsg;
      newMsg.m_sMessage = m_sMessage;
      newMsg.m_TriggerState = ezTriggerState::Deactivated;

      m_TriggerEventSender.PostEventMessage(newMsg, this, GetOwner()->GetParent(), ezTime::Zero(), ezObjectMsgQueueType::PostTransform);
    }
  }
}
