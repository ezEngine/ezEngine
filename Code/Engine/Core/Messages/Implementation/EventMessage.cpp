#include <CorePCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezEventMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventMessage, 1, ezRTTIDefaultAllocator<ezEventMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgGenericEvent);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgGenericEvent, 1, ezRTTIDefaultAllocator<ezMsgGenericEvent>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Message", m_sMessage),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
    new ezAutoGenVisScriptMsgSender,
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_CHECK_AT_COMPILETIME(sizeof(ezEventMessageSender<ezEventMessage>) == 8);

namespace ezInternal
{
  void EventMessageSenderHelper::SendMessage(ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg)
  {
    ezWorld* pWorld = pSenderComponent->GetWorld();
    ezComponent* pReceiverComponent = nullptr;
    if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
    {
      pReceiverComponent->SendMessage(msg);
    }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::SendMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, ezEventMessage& msg)
  {
    const ezWorld* pWorld = pSenderComponent->GetWorld();
    const ezComponent* pReceiverComponent = nullptr;
    if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
    {
      pReceiverComponent->SendMessage(msg);
    }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::PostMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, const ezEventMessage& msg, ezObjectMsgQueueType::Enum queueType)
  {
    if (!hReceiver.IsInvalidated())
    {
      const ezWorld* pWorld = pSenderComponent->GetWorld();
      pWorld->PostMessage(hReceiver, msg, queueType);
    }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::PostMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::PostMessage(const ezComponent* pSenderComponent, ezComponentHandle hReceiver, const ezEventMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay)
  {
    if (!hReceiver.IsInvalidated())
    {
      const ezWorld* pWorld = pSenderComponent->GetWorld();
      pWorld->PostMessage(hReceiver, msg, queueType, delay);
    }
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    else if (msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::PostMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }
} // namespace ezInternal



EZ_STATICLINK_FILE(Core, Core_Messages_Implementation_EventMessage);
