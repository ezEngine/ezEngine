#pragma once

template <typename EventData, typename PassThrough>
ezEventBase<EventData, PassThrough>::ezEventBase(ezIAllocator* pAllocator) : m_EventHandlers(pAllocator)
{
  m_bBroadcasting = false;
}

/// A callback can be registered multiple times with different pass-through data (or even with the same,
/// though that is less useful).
template <typename EventData, typename PassThrough>
void ezEventBase<EventData, PassThrough>::AddEventHandler(ezEventHandler callback, PassThrough pPassThrough)
{
  EZ_ASSERT(callback != NULL, "ezEvent::AddEventHandler: Callback may not be NULL."); 

  ezEventReceiver er;
  er.m_Callback = callback;
  er.m_pPassThrough = pPassThrough;

  m_EventHandlers.Append(er);
}

/// Use exactly the same combination of callback/pass-through-data to unregister an event handlers.
/// Otherwise an error occurs.
template <typename EventData, typename PassThrough>
void ezEventBase<EventData, PassThrough>::RemoveEventHandler(ezEventHandler callback, PassThrough pPassThrough)
{
  EZ_ASSERT(callback != NULL, "ezEvent::RemoveEventHandler: Callback may not be NULL.");

  for (ezUInt32 ui = 0; ui < m_EventHandlers.GetCount(); ++ui)
  {
    if ((m_EventHandlers[ui].m_Callback == callback) && (m_EventHandlers[ui].m_pPassThrough == pPassThrough))
    {
      m_EventHandlers.RemoveAt(ui);
      return;
    }
  }

  EZ_REPORT_FAILURE("ezEvent::RemoveEventHandler: Callback %p with pass-through data %p has not been registered or already been unregistered.", callback, pPassThrough);
}

/// The notification is sent to all event handlers in the order that they were registered.
template <typename EventData, typename PassThrough>
void ezEventBase<EventData, PassThrough>::Broadcast(EventData pEventData)
{
  EZ_ASSERT(!m_bBroadcasting, "The event has been triggered recursively or from several threads simultaniously.");

  if (m_bBroadcasting)
    return;

  m_bBroadcasting = true;

  for (ezUInt32 ui = 0; ui < m_EventHandlers.GetCount(); ++ui)
    m_EventHandlers[ui].m_Callback (pEventData, m_EventHandlers[ui].m_pPassThrough);

  m_bBroadcasting = false;
}


template <typename EventData, typename PassThrough, typename AllocatorWrapper>
ezEvent<EventData, PassThrough, AllocatorWrapper>::ezEvent() : ezEventBase<EventData, PassThrough>(AllocatorWrapper::GetAllocator())
{
}

template <typename EventData, typename PassThrough, typename AllocatorWrapper>
ezEvent<EventData, PassThrough, AllocatorWrapper>::ezEvent(ezIAllocator* pAllocator) : ezEventBase<EventData, PassThrough>(pAllocator)
{
}
