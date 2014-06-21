#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

static void BroadcastMemoryStats()
{
  ezUInt64 uiTotalAllocations = 0;

  for (auto it = ezMemoryTracker::GetIterator(); it.IsValid(); ++it)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('MEM', 'STAT');
    msg.GetWriter() << it.Name();
    msg.GetWriter() << it.Stats();

    uiTotalAllocations += it.Stats().m_uiNumAllocations;

    ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
  }

  static ezUInt64 uiLastTotalAllocations = 0;

  ezStringBuilder s;

  s.Format("%lli", uiTotalAllocations - uiLastTotalAllocations);
  ezStats::SetStat("App/Allocs Per Frame", s.GetData());

  uiLastTotalAllocations = uiTotalAllocations;
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

