#include <PCH.h>
#include <Foundation/Communication/GlobalEvent.h>

static ezGlobalEvent::EventMap s_LastState;

static void SendGlobalEventTelemetry(const char* szEvent, const ezGlobalEvent::EventData& ed)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage msg;
  msg.SetMessageID('EVNT', 'DATA');
  msg.GetWriter() << szEvent;
  msg.GetWriter() << ed.m_uiNumTimesFired;
  msg.GetWriter() << ed.m_uiNumEventHandlersRegular;
  msg.GetWriter() << ed.m_uiNumEventHandlersOnce;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
}

static void SendAllGlobalEventTelemetry()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    ezTelemetryMessage msg;
    ezTelemetry::Broadcast(ezTelemetry::Reliable, 'EVNT', 'CLR', NULL, 0);
  }

  ezGlobalEvent::UpdateGlobalEventStatistics();

  s_LastState = ezGlobalEvent::GetEventStatistics();

  for (ezGlobalEvent::EventMap::ConstIterator it = s_LastState.GetIterator(); it.IsValid(); ++it)
  {
    SendGlobalEventTelemetry(it.Key().GetData(), it.Value());
  }
}

static void SendChangedGlobalEventTelemetry()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  static ezTime LastUpdate = ezSystemTime::Now();

  if ((ezSystemTime::Now() - LastUpdate).GetSeconds() < 0.5)
    return;

  LastUpdate = ezSystemTime::Now();

  ezGlobalEvent::UpdateGlobalEventStatistics();

  const ezGlobalEvent::EventMap& data = ezGlobalEvent::GetEventStatistics();

  if (data.GetCount() != s_LastState.GetCount())
  {
    SendAllGlobalEventTelemetry();
    return;
  }

  bool bChange = false;

  for (ezGlobalEvent::EventMap::ConstIterator it = data.GetIterator(); it.IsValid(); ++it)
  {
    const ezGlobalEvent::EventData& ed1 = it.Value();
    const ezGlobalEvent::EventData& ed2 = s_LastState[it.Key()];

    /// \todo There is no ezMemoryUtils::MemCompare, only IsEqual, which is not the same (cannot compare arbirary memory of classes)
    if (memcmp(&ed1, &ed2, sizeof(ezGlobalEvent::EventData)) != 0)
    {
      bChange = true;
      SendGlobalEventTelemetry(it.Key().GetData(), it.Value());
    }
  }

  if (bChange)
    s_LastState = data;
}

static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e, void* pPassThrough)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendAllGlobalEventTelemetry();
    break;
  case ezTelemetry::TelemetryEventData::PerFrameUpdate:
    SendChangedGlobalEventTelemetry();
    break;
  }
}

void AddGlobalEventHandler()
{
  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
}

void RemoveGlobalEventHandler()
{
  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);
}

