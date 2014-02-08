#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Time/Clock.h>

static void TimeEventHandler(const ezClock::EventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetryMessage Msg;
  Msg.SetMessageID('TIME', 'UPDT');
  Msg.GetWriter() << e.m_szClockName;
  Msg.GetWriter() << ezTime::Now();
  Msg.GetWriter() << e.m_RawTimeStep;
  Msg.GetWriter() << e.m_SmoothedTimeStep;

  ezTelemetry::Broadcast(ezTelemetry::Unreliable, Msg);
}

void AddTimeEventHandler()
{
  ezClock::AddEventHandler(TimeEventHandler);
}

void RemoveTimeEventHandler()
{
  ezClock::RemoveEventHandler(TimeEventHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Time);

