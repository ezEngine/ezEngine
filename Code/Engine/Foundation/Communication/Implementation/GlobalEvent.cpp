#include <Foundation/PCH.h>
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

ezGlobalEvent::ezGlobalEvent(const char* szEventName, EZ_GLOBAL_EVENT_HANDLER Handler, bool bOnlyOnce)
{
  m_szEventName = szEventName;
  m_bOnlyOnce = bOnlyOnce;
  m_bHasBeenFired = false;
  m_EventHandler = Handler;
}

void ezGlobalEvent::Broadcast(const char* szEventName, ezVariant p1, ezVariant p2, ezVariant p3, ezVariant p4)
{
  ezGlobalEvent* pHandler = ezGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    if (ezStringUtils::IsEqual(pHandler->m_szEventName, szEventName))
    {
      if (!pHandler->m_bOnlyOnce || !pHandler->m_bHasBeenFired)
      {
        pHandler->m_bHasBeenFired = true;

        pHandler->m_EventHandler(p1, p2, p3, p4);
      }
    }

    pHandler = pHandler->GetNextInstance();
  }


  EventData& ed = s_KnownEvents[szEventName]; // this will make sure to record all fired events, even if there are no handlers for them
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
    EventData& ed = s_KnownEvents[pHandler->m_szEventName];
    
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
    ezLog::Info("Event: '%s', Num Handlers Regular / Once: %i / %i, Num Times Fired: %i", it.Key().GetData(), it.Value().m_uiNumEventHandlersRegular, it.Value().m_uiNumEventHandlersOnce, it.Value().m_uiNumTimesFired);

    ++it;
  }
}



EZ_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_GlobalEvent);

