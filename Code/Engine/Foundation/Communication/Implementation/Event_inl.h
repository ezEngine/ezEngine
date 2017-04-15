#pragma once

template <typename EventData, typename MutexType>
ezEventBase<EventData, MutexType>::ezEventBase(ezAllocatorBase* pAllocator) : m_EventHandlers(pAllocator)
{
  m_uiRecursionDepth = 0;
}

/// A callback can be registered multiple times with different pass-through data (or even with the same,
/// though that is less useful).
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::AddEventHandler(Handler handler) const
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_DEV(!m_EventHandlers.Contains(handler), "Event handler cannot be added twice");

  m_EventHandlers.PushBack(handler);
}

/// Use exactly the same combination of callback/pass-through-data to unregister an event handlers.
/// Otherwise an error occurs.
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::RemoveEventHandler(Handler handler) const
{
  EZ_LOCK(m_Mutex);

  bool bResult = m_EventHandlers.Remove(handler);
  EZ_IGNORE_UNUSED(bResult);
  EZ_ASSERT_DEV(bResult, "ezEvent::RemoveEventHandler: Handler has not been registered or already been unregistered.");
}

/// The notification is sent to all event handlers in the order that they were registered.
template <typename EventData, typename MutexType>
void ezEventBase<EventData, MutexType>::Broadcast(EventData eventData, ezUInt8 uiMaxRecursionDepth)
{
  EZ_LOCK(m_Mutex);

  EZ_ASSERT_DEV(m_uiRecursionDepth <= uiMaxRecursionDepth, "The event has been triggered recursively or from several threads simultaneously.");

  if (m_uiRecursionDepth > uiMaxRecursionDepth)
    return;

  ++m_uiRecursionDepth;

  for (ezUInt32 ui = 0; ui < m_EventHandlers.GetCount(); ++ui)
    m_EventHandlers[ui](eventData);

  --m_uiRecursionDepth;
}


template <typename EventData, typename MutexType, typename AllocatorWrapper>
ezEvent<EventData, MutexType, AllocatorWrapper>::ezEvent() : ezEventBase<EventData, MutexType>(AllocatorWrapper::GetAllocator())
{
}

template <typename EventData, typename MutexType, typename AllocatorWrapper>
ezEvent<EventData, MutexType, AllocatorWrapper>::ezEvent(ezAllocatorBase* pAllocator) : ezEventBase<EventData, MutexType>(pAllocator)
{
}

