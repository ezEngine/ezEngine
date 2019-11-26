#include <InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

#include <GameEngine/GameApplication/GameApplicationBase.h>

namespace MemoryDetail
{

  static void BroadcastMemoryStats()
  {
    ezUInt64 uiTotalAllocations = 0;

    {
      ezTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'BGN');
      ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
    }
       
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

    {
      ezTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'END');
      ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
    }

    static ezUInt64 uiLastTotalAllocations = 0;

    ezStringBuilder s;

    ezStats::SetStat("App/Allocs Per Frame", uiTotalAllocations - uiLastTotalAllocations);

    uiLastTotalAllocations = uiTotalAllocations;
  }

  static void PerframeUpdateHandler(const ezGameApplicationExecutionEvent& e)
  {
    if (!ezTelemetry::IsConnectedToClient())
      return;

    switch (e.m_Type)
    {
      case ezGameApplicationExecutionEvent::Type::AfterPresent:
        BroadcastMemoryStats();
        break;

      default:
        break;
    }
  }
}


void AddMemoryEventHandler()
{
  // We're handling the per frame update by a different event since
  // using ezTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the ezStats and ezTelemetry system.
  ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(MemoryDetail::PerframeUpdateHandler);
}

void RemoveMemoryEventHandler()
{
  ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(MemoryDetail::PerframeUpdateHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Memory);
