#include <InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

namespace MemoryDetail
{

  static void BroadcastMemoryStats()
  {
    ezUInt64 uiTotalAllocations = 0;

    for (auto it = ezMemoryTracker::GetIterator(); it.IsValid(); ++it)
    {
      ezTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'STAT');
      msg.GetWriter() << it.Id().m_Data;
      msg.GetWriter() << it.Name();
      msg.GetWriter() << (it.ParentId().IsInvalidated() ? ezInvalidIndex : it.ParentId().m_Data);
      msg.GetWriter() << it.Stats();

      uiTotalAllocations += it.Stats().m_uiNumAllocations;

      ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
    }

    static ezUInt64 uiLastTotalAllocations = 0;

    ezStringBuilder s;

    ezStats::SetStat("App/Allocs Per Frame", uiTotalAllocations - uiLastTotalAllocations);

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

      default:
        break;
    }
  }
}


void AddMemoryEventHandler()
{
  ezTelemetry::AddEventHandler(MemoryDetail::TelemetryEventsHandler);
}

void RemoveMemoryEventHandler()
{
  ezTelemetry::RemoveEventHandler(MemoryDetail::TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Memory);
