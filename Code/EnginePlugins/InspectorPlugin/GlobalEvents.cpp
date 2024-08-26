#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Communication/Telemetry.h>

#include <Core/GameApplication/GameApplicationBase.h>

static ezGlobalEvent::EventMap s_LastState;

static void SendGlobalEventTelemetry(ezStringView sEvent, const ezGlobalEvent::EventData& ed)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage msg;
  msg.SetMessageID('EVNT', 'DATA');
  msg.GetWriter() << sEvent;
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
    ezTelemetry::Broadcast(ezTelemetry::Reliable, 'EVNT', ' CLR', nullptr, 0);
  }

  ezGlobalEvent::UpdateGlobalEventStatistics();

  s_LastState = ezGlobalEvent::GetEventStatistics();

  for (ezGlobalEvent::EventMap::ConstIterator it = s_LastState.GetIterator(); it.IsValid(); ++it)
  {
    SendGlobalEventTelemetry(it.Key(), it.Value());
  }
}

static void SendChangedGlobalEventTelemetry()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  static ezTime LastUpdate = ezTime::Now();

  if ((ezTime::Now() - LastUpdate).GetSeconds() < 0.5)
    return;

  LastUpdate = ezTime::Now();

  ezGlobalEvent::UpdateGlobalEventStatistics();

  const ezGlobalEvent::EventMap& data = ezGlobalEvent::GetEventStatistics();

  if (data.GetCount() != s_LastState.GetCount())
  {
    SendAllGlobalEventTelemetry();
    return;
  }

  for (ezGlobalEvent::EventMap::ConstIterator it = data.GetIterator(); it.IsValid(); ++it)
  {
    const ezGlobalEvent::EventData& currentEventData = it.Value();
    ezGlobalEvent::EventData& lastEventData = s_LastState[it.Key()];

    if (ezMemoryUtils::Compare(&currentEventData, &lastEventData) != 0)
    {
      SendGlobalEventTelemetry(it.Key().GetData(), it.Value());

      lastEventData = currentEventData;
    }
  }
}

namespace GlobalEventsDetail
{
  static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
  {
    if (!ezTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case ezTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllGlobalEventTelemetry();
        break;
      case ezTelemetry::TelemetryEventData::DisconnectedFromClient:
      {
        ezGlobalEvent::EventMap tmp;
        s_LastState.Swap(tmp);
        break;
      }
      default:
        break;
    }
  }

  static void PerframeUpdateHandler(const ezGameApplicationExecutionEvent& e)
  {
    if (!ezTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case ezGameApplicationExecutionEvent::Type::AfterPresent:
        SendChangedGlobalEventTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace GlobalEventsDetail

void AddGlobalEventHandler()
{
  ezTelemetry::AddEventHandler(GlobalEventsDetail::TelemetryEventsHandler);

  // We're handling the per frame update by a different event since
  // using ezTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the ezStats and ezTelemetry system.
  if (ezGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }
}

void RemoveGlobalEventHandler()
{
  if (ezGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(GlobalEventsDetail::PerframeUpdateHandler);
  }

  ezTelemetry::RemoveEventHandler(GlobalEventsDetail::TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_GlobalEvents);
