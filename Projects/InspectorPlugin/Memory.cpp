#include <PCH.h>

static void BroadcastMemoryStats()
{
  ezIAllocator* pAllocator = ezIAllocator::GetFirstInstance();

  while (pAllocator)
  {
    ezIAllocator::Stats s;
    pAllocator->GetStats(s);

    ezTelemetryMessage msg;
    msg.SetMessageID('MEM', 'STAT');
    msg.GetWriter() << pAllocator->GetName();
    msg.GetWriter() << s;

    ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);

    pAllocator = pAllocator->GetNextInstance();
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
