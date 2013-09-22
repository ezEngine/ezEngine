#include <PCH.h>

void StatsEventHandler(const ezStats::StatsEventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
  case ezStats::StatsEventData::Set:
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('STAT', 'SET');
      msg.GetWriter() << e.m_szStatName;
      msg.GetWriter() << e.m_szNewStatValue;

      ezTelemetry::Broadcast(ezTelemetry::Unreliable, msg);
    }
    break;
  case ezStats::StatsEventData::Remove:
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('STAT', 'DEL');
      msg.GetWriter() << e.m_szStatName;

      ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
    }
    break;
  }
}


static void SendAllStatsTelemetry()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  for (ezStats::MapType::ConstIterator it = ezStats::GetAllStats().GetIterator(); it.IsValid(); ++it)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('STAT', 'SET');
    msg.GetWriter() << it.Key().GetData();
    msg.GetWriter() << it.Value().GetData();

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }
}

static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendAllStatsTelemetry();
    break;
  case ezTelemetry::TelemetryEventData::PerFrameUpdate:
    {
      const ezTime Now = ezSystemTime::Now();

      static ezTime LastTime = Now;
      static ezTime LastFPS = Now;
      static ezUInt32 uiFPS = 0;
      ++uiFPS;

      const ezTime TimeDiff = Now - LastTime;

      ezStringBuilder s;
      s.Format("%.2fms", TimeDiff.GetMilliSeconds());
      ezStats::SetStat("App/FrameTime", s.GetData());

      LastTime = Now;

      if ((Now - LastFPS).GetSeconds() >= 1.0)
      {
      s.Format("%u", uiFPS);
      ezStats::SetStat("App/FPS", s.GetData());

        LastFPS = Now;
        uiFPS = 0;
      }
    }
    break;
  }
}


void AddStatsEventHandler()
{
  ezStats::AddEventHandler(StatsEventHandler);

  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
}

void RemoveStatsEventHandler()
{
  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);

  ezStats::RemoveEventHandler(StatsEventHandler);
}

