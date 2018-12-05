#pragma once

template <typename EventData, typename MutexType>
ezEventBase<EventData, MutexType>::ezEventBase(ezAllocatorBase* pAllocator)
    : m_EventHandlers(pAllocator)
{
  m_uiRecursionDepth = 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_pSelf = this;
#endif
}

template <typename EventData, typename MutexType>
ezEventBase<EventData, MutexType>::~ezEventBase()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_ASSERT_ALWAYS(m_pSelf == this, "The ezEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif
}

/// A callback can be registered multiple times with different pass-through data (or even with the same,
/// though that is less useful).
template <typename EventData, typename MutexType>
ezEventSubscriptionID ezEventBase<EventData, MutexType>::AddEventHandler(Handler handler) const
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_DEV(!HasEventHandler(handler), "Event handler cannot be added twice");

  auto& item = m_EventHandlers.ExpandAndGetRef();
  item.m_Handler = handler;
  item.m_SubscriptionID = ++m_NextSubscriptionID;

  return item.m_SubscriptionID;
}

template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::AddEventHandler(Handler handler, Unsubscriber& unsubscriber) const
{
  EZ_LOCK(m_Mutex);

  unsubscriber.Unsubscribe();
  unsubscriber.m_pEvent = this;
  unsubscriber.m_SubscriptionID = AddEventHandler(handler);
}


/// Use exactly the same combination of callback/pass-through-data to unregister an event handlers.
/// Otherwise an error occurs.
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::RemoveEventHandler(Handler handler) const
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_Handler == handler)
    {
      m_EventHandlers.RemoveAtAndCopy(idx);
      return;
    }
  }

  EZ_ASSERT_DEV(false, "ezEvent::RemoveEventHandler: Handler has not been registered or already been unregistered.");
}

template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::RemoveEventHandler(ezEventSubscriptionID& id) const
{
  if (id == 0)
    return;

  EZ_LOCK(m_Mutex);

  for (ezUInt32 idx = 0; idx < m_EventHandlers.GetCount(); ++idx)
  {
    if (m_EventHandlers[idx].m_SubscriptionID == id)
    {
      m_EventHandlers.RemoveAtAndCopy(idx);
      id = 0;
      return;
    }
  }

  EZ_ASSERT_DEV(false, "ezEvent::RemoveEventHandler: Invalid subscription ID '{0}'.", (ezInt32)id);
}

template <typename EventData, typename MutexType>
bool ezEventBase<EventData, MutexType>::HasEventHandler(Handler handler) const
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = 0; i < m_EventHandlers.GetCount(); ++i)
  {
    if (m_EventHandlers[i].m_Handler == handler)
      return true;
  }

  return false;
}

/// The notification is sent to all event handlers in the order that they were registered.
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::Broadcast(EventData eventData, ezUInt8 uiMaxRecursionDepth) const
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth,
                "The event has been triggered recursively or from several threads simultaneously.");

  if (m_uiRecursionDepth > uiMaxRecursionDepth)
    return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  EZ_ASSERT_ALWAYS(m_pSelf == this, "The ezEvent was relocated in memory. This is not allowed, as it breaks the Unsubscribers.");
#endif

  ++m_uiRecursionDepth;

  for (ezUInt32 ui = 0; ui < m_EventHandlers.GetCount(); ++ui)
    m_EventHandlers[ui].m_Handler(eventData);

  --m_uiRecursionDepth;
}


template <typename EventData, typename MutexType, typename AllocatorWrapper>
ezEvent<EventData, MutexType, AllocatorWrapper>::ezEvent()
    : ezEventBase<EventData, MutexType>(AllocatorWrapper::GetAllocator())
{
}

template <typename EventData, typename MutexType, typename AllocatorWrapper>
ezEvent<EventData, MutexType, AllocatorWrapper>::ezEvent(ezAllocatorBase* pAllocator)
    : ezEventBase<EventData, MutexType>(pAllocator)
{
}
