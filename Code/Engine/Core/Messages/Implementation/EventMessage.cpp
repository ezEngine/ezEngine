#include <Core/CorePCH.h>

#include <Core/Messages/EventMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezEventMessage);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventMessage, 1, ezRTTIDefaultAllocator<ezEventMessage>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

EZ_CHECK_AT_COMPILETIME(sizeof(ezEventMessageSender<ezEventMessage>) == 16);

namespace ezInternal
{
  void EventMessageSenderHelper::SendEventMessage(ezComponent* pSenderComponent, ezArrayPtr<ezComponentHandle> receivers, ezEventMessage& msg)
  {
    ezWorld* pWorld = pSenderComponent->GetWorld();
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    for (auto hReceiver : receivers)
    {
      ezComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        pReceiverComponent->SendMessage(msg);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::SendEventMessage(const ezComponent* pSenderComponent, ezArrayPtr<ezComponentHandle> receivers, ezEventMessage& msg)
  {
    const ezWorld* pWorld = pSenderComponent->GetWorld();
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    for (auto hReceiver : receivers)
    {
      const ezComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        pReceiverComponent->SendMessage(msg);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", msg.GetId());
    }
#endif
  }

  void EventMessageSenderHelper::PostEventMessage(const ezComponent* pSenderComponent, ezArrayPtr<ezComponentHandle> receivers, const ezEventMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType)
  {
    if (!receivers.IsEmpty())
    {
      const ezWorld* pWorld = pSenderComponent->GetWorld();
      for (auto hReceiver : receivers)
      {
        pWorld->PostMessage(hReceiver, msg, delay, queueType);
      }
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
