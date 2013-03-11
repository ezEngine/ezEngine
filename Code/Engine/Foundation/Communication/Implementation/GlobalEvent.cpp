#include <Foundation/PCH.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Logging/Log.h>

EZ_ENUMERABLE_CLASS_IMPLEMENTATION(ezGlobalEvent);

ezStatic<ezMap<ezStaticString<64>, ezGlobalEvent::EventData > > ezGlobalEvent::s_KnownEvents;

ezGlobalEvent::EventData::EventData()
{
//  m_pFirstHandler = NULL;
  m_uiNumTimesFired = 0;
  m_uiNumEventHandlersOnce = 0;
  m_uiNumEventHandlersRegular = 0;
}

ezGlobalEvent::ezGlobalEvent(const char* szEventName, EZ_GLOBAL_EVENT_HANDLER Handler, bool bOnlyOnce)
{
  //EventData& ed = s_KnownEvents.GetStatic()[szEventName];

  m_szEventName = szEventName;
  m_bOnlyOnce = bOnlyOnce;
  m_bHasBeenFired = false;
  m_EventHandler = Handler;
  //m_pNextHandler = ed.m_pFirstHandler;
  

  //ed.m_pFirstHandler = this;

  //if (m_bOnlyOnce)
  //  ed.m_uiNumEventHandlersOnce++;
  //else
  //  ed.m_uiNumEventHandlersRegular++;
}

ezGlobalEvent::~ezGlobalEvent()
{
  //EventData& ed = s_KnownEvents.GetStatic()[m_szEventName];

  //if (m_bOnlyOnce)
  //  ed.m_uiNumEventHandlersOnce--;
  //else
  //  ed.m_uiNumEventHandlersRegular--;


  //RemoveHandlerFromQueue();
}

//void ezGlobalEvent::RemoveHandlerFromQueue()
//{
//  EventData& ed = s_KnownEvents.GetStatic()[m_szEventName];
//
//  ezGlobalEvent* pPrev = NULL;
//  ezGlobalEvent* pCur = ed.m_pFirstHandler;
//    
//  while (pCur)
//  {
//    if (pCur == this)
//    {
//      if (pPrev == NULL)
//        ed.m_pFirstHandler = m_pNextHandler;
//      else
//        pPrev->m_pNextHandler = m_pNextHandler;
//        
//      break;
//    }
//      
//    pPrev = pCur;
//    pCur = pCur->m_pNextHandler;
//  }
//}

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


  EventData& ed = s_KnownEvents.GetStatic()[szEventName]; // this will make sure to record all fired events, even if there are no handlers for them
  ed.m_uiNumTimesFired++;

  //ezGlobalEvent* pHandler = ed.m_pFirstHandler;

  //while (pHandler)
  {
    // send the event
    //pHandler->m_EventHandler(p1, p2, p3, p4);

    //ezGlobalEvent* pNext = pHandler->m_pNextHandler;

    //// if this handler was supposed to handle the event only once, just remove it from the handler queue
    //if (pHandler->m_bOnlyOnce)
    //  pHandler->RemoveHandlerFromQueue();
    //
    //pHandler = pNext;
  }
}

void ezGlobalEvent::PrintGlobalEventStatistics()
{
  EZ_LOG_BLOCK("Globel Event Statistics");

  ezGlobalEvent* pHandler = ezGlobalEvent::GetFirstInstance();

  while (pHandler)
  {
    EventData& ed = s_KnownEvents.GetStatic()[pHandler->m_szEventName];
    
    if (pHandler->m_bOnlyOnce)
      ++ed.m_uiNumEventHandlersOnce;
    else
      ++ed.m_uiNumEventHandlersRegular;

    pHandler = pHandler->GetNextInstance();
  }

  ezMap<ezStaticString<64>, EventData >::Iterator it = s_KnownEvents.GetStatic().GetIterator();

  while (it.IsValid())
  {
    ezLog::Info("Event: '%s', Num Handlers Regular / Once: %i / %i, Num Times Fired: %i", it.Key().GetData(), it.Value().m_uiNumEventHandlersRegular, it.Value().m_uiNumEventHandlersOnce, it.Value().m_uiNumTimesFired);

    ++it;
  }
}

