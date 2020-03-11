#pragma once

#include <Foundation/Types/ScopeExit.h>

template <typename EventData, typename MutexType, bool CopyOnBroadcast>
ezEventBase<EventData, MutexType, CopyOnBroadcast>::ezEventBase(ezAllocatorBase* pAllocator)
  : m_EventHandlers(pAllocator)
{
  m_uiRecursionDepth = 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_pSelf = this;
#endif
}

template <typename EventData, typename MutexType, bool CopyOnBroadcast>
ezEventBase<EventData, MutexType, CopyOnBroadcast>::~ezEventBase()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_ASSERT_ALWAYS(m_pSelf == this, "The ezEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif
}

/// A callback can be registered multiple times with different pass-through data (or even with the same,
/// though that is less useful).
template <typename EventData, typename MutexType, bool CopyOnBroadcast>
ezEventSubscriptionID ezEventBase<EventData, MutexType, CopyOnBroadcast>::AddEventHandler(Handler handler) const
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_DEV(!handler.IsComparable() || !HasEventHandler(handler), "Event handler cannot be added twice");

  auto& item = m_EventHandlers.ExpandAndGetRef();
  item.m_Handler = std::move(handler);
  item.m_SubscriptionID = ++m_NextSubscriptionID;

  return item.m_SubscriptionID;
}

template <typename EventData, typename MutexType, bool CopyOnBroadcast>
void ezEventBase<EventData, MutexType, CopyOnBroadcast>::AddEventHandler(Handler handler, Unsubscriber& unsubscriber) const
{
  EZ_LOCK(m_Mutex);

  unsubscriber.Unsubscribe();
  unsubscriber.m_pEvent = this;
  unsubscriber.m_SubscriptionID = AddEventHandler(std::move(handler));
}


/// Use exactly the same combination of callback/pass-through-data to unregister an event handlers.
/// Otherwise an error occurs.
template <typename EventData, typename MutexType, bool CopyOnBroadcast>
void ezEventBase<EventData, MutexType, CopyOnBroadcast>::RemoveEventHandler(const Handler& handler) const
{
  EZ_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be removed via function pointer. Use an ezEventSubscriptionID instead.");

  EZ_LOCK(m_Mutex);

  for (ezUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_Handler.IsEqualIfComparable(handler))
    {
      EZ_ASSERT_DEV(!m_bCurrentlyBroadcasting, "Can not remove event handlers while broadcasting. Use the CopyOnBroadcast = true version to modify the event during broadcasting.");
      m_EventHandlers.RemoveAtAndCopy(idx);
      return;
    }
  }

  EZ_ASSERT_DEV(false, "ezEvent::RemoveEventHandler: Handler has not been registered or already been unregistered.");
}

template <typename EventData, typename MutexType, bool CopyOnBroadcast>
void ezEventBase<EventData, MutexType, CopyOnBroadcast>::RemoveEventHandler(ezEventSubscriptionID& id) const
{
  if (id == 0)
    return;

  EZ_LOCK(m_Mutex);

  for (ezUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_SubscriptionID == id)
    {
      EZ_ASSERT_DEV(!m_bCurrentlyBroadcasting, "Can not remove event handlers while broadcasting. Use the CopyOnBroadcast = true version to modify the event during broadcasting.");
      m_EventHandlers.RemoveAtAndCopy(idx);
      id = 0;
      return;
    }
  }

  EZ_ASSERT_DEV(false, "ezEvent::RemoveEventHandler: Invalid subscription ID '{0}'.", (ezInt32)id);
}

template <typename EventData, typename MutexType, bool CopyOnBroadcast>
bool ezEventBase<EventData, MutexType, CopyOnBroadcast>::HasEventHandler(const Handler& handler) const
{
  EZ_ASSERT_DEV(handler.IsComparable(), "Lambdas that capture data cannot be checked via function pointer. Use an ezEventSubscriptionID instead.");

  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_EventHandlers.GetCount(); ++i)
  {
    if (m_EventHandlers[i].m_Handler.IsEqualIfComparable(handler))
      return true;
  }

  return false;
}

template <typename EventData, typename MutexType, bool CopyOnBroadcast>
void ezEventBase<EventData, MutexType, CopyOnBroadcast>::Clear()
{
  m_EventHandlers.Clear();
}

/// The notification is sent to all event handlers in the order that they were registered.
template <typename EventData, typename MutexType, bool CopyOnBroadcast>
void ezEventBase<EventData, MutexType, CopyOnBroadcast>::Broadcast(EventData eventData, ezUInt8 uiMaxRecursionDepth)
{
  if constexpr (CopyOnBroadcast == false)
  {
    EZ_LOCK(m_Mutex);

    EZ_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth,
      "The event has been triggered recursively or from several threads simultaneously.");

    if (m_uiRecursionDepth > uiMaxRecursionDepth)
      return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    EZ_ASSERT_ALWAYS(m_pSelf == this, "The ezEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
    if (m_uiRecursionDepth == 0)
    {
      m_bCurrentlyBroadcasting = true;
    }
#endif

    m_uiRecursionDepth.Increment();

    EZ_SCOPE_EXIT(m_uiRecursionDepth.Decrement());

    for (ezUInt32 ui = 0; ui < m_EventHandlers.GetCount(); ++ui)
      m_EventHandlers[ui].m_Handler(eventData);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    if (m_uiRecursionDepth == 0)
    {
      m_bCurrentlyBroadcasting = false;
    }
#endif
  }
  else
  {
    ezHybridArray<HandlerData, 16> eventHandlers;
    {
      EZ_LOCK(m_Mutex);

      EZ_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth,
        "The event has been triggered recursively or from several threads simultaneously.");

      if (m_uiRecursionDepth > uiMaxRecursionDepth)
        return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      EZ_ASSERT_ALWAYS(m_pSelf == this, "The ezEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

      eventHandlers = m_EventHandlers;

      m_uiRecursionDepth.Increment();
    }

    EZ_SCOPE_EXIT(m_uiRecursionDepth.Decrement());

    ezUInt32 uiHandlerCount = eventHandlers.GetCount();
    for (ezUInt32 ui = 0; ui < uiHandlerCount; ++ui)
    {
      eventHandlers[ui].m_Handler(eventData);
    }
  }
}


template <typename EventData, typename MutexType, typename AllocatorWrapper, bool CopyOnBroadcast>
ezEvent<EventData, MutexType, AllocatorWrapper, CopyOnBroadcast>::ezEvent()
  : ezEventBase<EventData, MutexType, CopyOnBroadcast>(AllocatorWrapper::GetAllocator())
{
}

template <typename EventData, typename MutexType, typename AllocatorWrapper, bool CopyOnBroadcast>
ezEvent<EventData, MutexType, AllocatorWrapper, CopyOnBroadcast>::ezEvent(ezAllocatorBase* pAllocator)
  : ezEventBase<EventData, MutexType, CopyOnBroadcast>(pAllocator)
{
}
