#include <Core/CorePCH.h>

#include <Core/Messages/EventMessageSender.h>
#include <Core/World/World.h>

namespace ezInternal
{
  template <typename World, typename GameObject>
  static void UpdateCachedReceivers(const ezMessage& msg, World& ref_world, const ezComponent* pSenderComponent, GameObject pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers)
  {
    if (inout_cachedReceivers.GetUserData<ezUInt32>() == 0)
    {
      using ComponentType = typename std::conditional<std::is_const<World>::value, const ezComponent*, ezComponent*>::type;

      ezHybridArray<ComponentType, 4> eventMsgHandlers;
      ref_world.FindEventMsgHandlers(msg, pSenderComponent, pSearchObject, eventMsgHandlers);

      for (auto pEventMsgHandler : eventMsgHandlers)
      {
        inout_cachedReceivers.PushBack(pEventMsgHandler->GetHandle());
      }

      inout_cachedReceivers.GetUserData<ezUInt32>() = 1;
    }
  }

  bool EventMessageSenderHelper::SendEventMessage(ezMessage& ref_msg, ezComponent* pSenderComponent, ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers)
  {
    ezWorld* pWorld = pSearchObject->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSenderComponent, pSearchObject, inout_cachedReceivers);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : inout_cachedReceivers)
    {
      ezComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  bool EventMessageSenderHelper::SendEventMessage(ezMessage& ref_msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers)
  {
    const ezWorld* pWorld = pSearchObject->GetWorld();
    UpdateCachedReceivers(ref_msg, *pWorld, pSenderComponent, pSearchObject, inout_cachedReceivers);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    bool bHandlerFound = false;
#endif

    bool bResult = false;
    for (auto hReceiver : inout_cachedReceivers)
    {
      const ezComponent* pReceiverComponent = nullptr;
      if (pWorld->TryGetComponent(hReceiver, pReceiverComponent))
      {
        bResult |= pReceiverComponent->SendMessage(ref_msg);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
        bHandlerFound = true;
#endif
      }
    }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
    if (!bHandlerFound && ref_msg.GetDebugMessageRouting())
    {
      ezLog::Warning("ezEventMessageSender::SendMessage: No event message handler found for message of type {0}.", ref_msg.GetId());
    }
#endif

    return bResult;
  }

  void EventMessageSenderHelper::PostEventMessage(const ezMessage& msg, const ezComponent* pSenderComponent, const ezGameObject* pSearchObject, ezSmallArray<ezComponentHandle, 1>& inout_cachedReceivers, ezTime delay, ezObjectMsgQueueType::Enum queueType)
  {
    const ezWorld* pWorld = pSearchObject->GetWorld();
    UpdateCachedReceivers(msg, *pWorld, pSenderComponent, pSearchObject, inout_cachedReceivers);

    if (!inout_cachedReceivers.IsEmpty())
    {
      for (auto hReceiver : inout_cachedReceivers)
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
