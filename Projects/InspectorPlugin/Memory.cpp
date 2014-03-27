#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>

static void BroadcastMemoryStats()
{
  for (auto it = ezMemoryTracker::GetIterator(); it.IsValid(); ++it)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('MEM', 'STAT');
    msg.GetWriter() << it.Name();
    msg.GetWriter() << it.Stats();

    ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
  }
}

static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::PerFrameUpdate:
    BroadcastMemoryStats();
    break;
  }
}


void AddMemoryEventHandler()
{
  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
}

void RemoveMemoryEventHandler()
{
  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Memory);

