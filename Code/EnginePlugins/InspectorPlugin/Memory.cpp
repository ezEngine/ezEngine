#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Utilities/Stats.h>

#include <Core/GameApplication/GameApplicationBase.h>

namespace MemoryDetail
{

  static void BroadcastMemoryStats()
  {
    ezUInt64 uiTotalAllocations = 0;
    ezUInt64 uiTotalPerFrameAllocationSize = 0;
    ezTime TotalPerFrameAllocationTime;

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
      uiTotalPerFrameAllocationSize += it.Stats().m_uiPerFrameAllocationSize;
      TotalPerFrameAllocationTime += it.Stats().m_PerFrameAllocationTime;

      ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
    }

    {
      ezTelemetryMessage msg;
      msg.SetMessageID(' MEM', 'END');
      ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
    }

    static ezUInt64 uiLastTotalAllocations = 0;

    ezStats::SetStat("App/Allocs Per Frame", uiTotalAllocations - uiLastTotalAllocations);
    ezStats::SetStat("App/Per Frame Alloc Size (byte)", uiTotalPerFrameAllocationSize);
    ezStats::SetStat("App/Per Frame Alloc Time", TotalPerFrameAllocationTime);

    uiLastTotalAllocations = uiTotalAllocations;

    ezMemoryTracker::ResetPerFrameAllocatorStats();
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
} // namespace MemoryDetail


void AddMemoryEventHandler()
{
  // We're handling the per frame update by a different event since
  // using ezTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the ezStats and ezTelemetry system.
  if (ezGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}

void RemoveMemoryEventHandler()
{
  if (ezGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(MemoryDetail::PerframeUpdateHandler);
  }
}


