#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/Stats.h>

static void StatsEventHandler(const ezStats::StatsEventData& e)
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
      const ezTime Now = ezTime::Now();

      static ezTime LastTime = Now;
      static ezTime LastFPS = Now;
      static ezUInt32 uiFPS = 0;
      ++uiFPS;

      const ezTime TimeDiff = Now - LastTime;

      ezStringBuilder s, s2;
      s.Format("%.2fms", TimeDiff.GetMilliseconds());
      ezStats::SetStat("App/FrameTime", s.GetData());

      LastTime = Now;

      if ((Now - LastFPS).GetSeconds() >= 1.0)
      {
      s.Format("%u", uiFPS);
      ezStats::SetStat("App/FPS", s.GetData());

        LastFPS = Now;
        uiFPS = 0;
      }

      s.Format("%i", ezOSThread::GetThreadCount());
      ezStats::SetStat("App/Active Threads", s.GetData());

      // Tasksystem Thread Utilization
      {
        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks); ++t)
        {
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::ShortTasks, t);

          s.Format("Utilization/ShortTasks_%i", t, uiFPS);
          s2.Format("%.2f%%", Utilization * 100.0);

          ezStats::SetStat(s.GetData(), s2.GetData());
        }

        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks); ++t)
        {
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::LongTasks, t);

          s.Format("Utilization/LongTasks_%i", t, uiFPS);
          s2.Format("%.2f%%", Utilization * 100.0);

          ezStats::SetStat(s.GetData(), s2.GetData());
        }

        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::FileAccess); ++t)
        {
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::FileAccess, t);

          s.Format("Utilization/FileTasks_%i", t, uiFPS);
          s2.Format("%.2f%%", Utilization * 100.0);

          ezStats::SetStat(s.GetData(), s2.GetData());
        }
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



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Stats);

