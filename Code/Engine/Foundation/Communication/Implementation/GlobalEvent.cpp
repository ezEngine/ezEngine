#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/Log.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezGlobalEvent);

ezGlobalEvent::EventMap ezGlobalEvent::s_KnownEvents;

ezGlobalEvent::EventData::EventData()
{
  m_uiNumTimesFired = 0;
  m_uiNumEventHandlersOnce = 0;
  m_uiNumEventHandlersRegular = 0;
}

ezGlobalEvent::ezGlobalEvent(ezStringView sEventName, EZ_GLOBAL_EVENT_HANDLER handler, bool bOnlyOnce)
{
  m_sEventName = sEventName;
  m_bOnlyOnce = bOnlyOnce;
  m_bHasBeenFired = false;
  m_EventHandler = handler;
}

void ezGlobalEvent::Broadcast(ezStringView sEventName, ezVariant p1, ezVariant p2, ezVariant p3, ezVariant p4)
{
  ezGlobalEvent* pHandler = ezGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    if (pHandler->m_sEventName == sEventName)
    {
      if (!pHandler->m_bOnlyOnce || !pHandler->m_bHasBeenFired)
      {
        pHandler->m_bHasBeenFired = true;

        pHandler->m_EventHandler(p1, p2, p3, p4);
      }
    }

    pHandler = pHandler->GetNextInstance();
  }


  EventData& ed = s_KnownEvents[sEventName]; // this will make sure to record all fired events, even if there are no handlers for them
  ed.m_uiNumTimesFired++;
}

void ezGlobalEvent::UpdateGlobalEventStatistics()
{
  for (EventMap::Iterator it = s_KnownEvents.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_uiNumEventHandlersRegular = 0;
    it.Value().m_uiNumEventHandlersOnce = 0;
  }

  ezGlobalEvent* pHandler = ezGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    EventData& ed = s_KnownEvents[pHandler->m_sEventName];

    if (pHandler->m_bOnlyOnce)
      ++ed.m_uiNumEventHandlersOnce;
    else
      ++ed.m_uiNumEventHandlersRegular;

    pHandler = pHandler->GetNextInstance();
  }
}

void ezGlobalEvent::PrintGlobalEventStatistics()
{
  UpdateGlobalEventStatistics();

  EZ_LOG_BLOCK("Global Event Statistics");

  EventMap::Iterator it = s_KnownEvents.GetIterator();

  while (it.IsValid())
  {
    ezLog::Info("Event: '{0}', Num Handlers Regular / Once: {1} / {2}, Num Times Fired: {3}", it.Key(), it.Value().m_uiNumEventHandlersRegular,
      it.Value().m_uiNumEventHandlersOnce, it.Value().m_uiNumTimesFired);

    ++it;
  }
}
