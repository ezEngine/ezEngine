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
  template <typename World, typename GameObject>
  static void UpdateCachedReceivers(const ezMessage& msg, World& world, GameObject pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_CachedReceivers)
  {
    if (inout_CachedReceivers.GetUserData<ezUInt32>() == 0)
    {
      using ComponentType = typename std::conditional<std::is_const<World>::value, const ezComponent*, ezComponent*>::type;

      ezHybridArray<ComponentType, 4> eventMsgHandlers;
      world.FindEventMsgHandlers(msg, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        inout_CachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      inout_CachedReceivers.GetUserData<ezUInt32>() = 1;
    }
  }

  void EventMessageSenderHelper::SendEventMessage(ezMessage& msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_CachedReceivers)
  {
    ezWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, inout_CachedReceivers);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    for (auto hReceiver : inout_CachedReceivers)
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

  void EventMessageSenderHelper::SendEventMessage(ezMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_CachedReceivers)
  {
    const ezWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, inout_CachedReceivers);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    for (auto hReceiver : inout_CachedReceivers)
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

  void EventMessageSenderHelper::PostEventMessage(const ezMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_CachedReceivers, ezTime delay, ezObjectMsgQueueType::Enum queueType)
  {
    const ezWorld* pWorld = pSenderComponent->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSearchObject, inout_CachedReceivers);

    if (!inout_CachedReceivers.IsEmpty())
    {
      for (auto hReceiver : inout_CachedReceivers)
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
