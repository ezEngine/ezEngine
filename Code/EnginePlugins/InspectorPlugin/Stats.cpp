#include <InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/GameApplication/GameApplicationBase.h>

static void StatsEventHandler(const ezStats::StatsEventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetry::TransmitMode Mode = ezTelemetry::Reliable;

  switch (e.m_EventType)
  {
    case ezStats::StatsEventData::Set:
      Mode = ezTelemetry::Unreliable;
      // fall-through
    case ezStats::StatsEventData::Add:
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('STAT', ' SET');
      msg.GetWriter() << e.m_szStatName;
      msg.GetWriter() << e.m_NewStatValue;
      msg.GetWriter() << ezTime::Now();

      ezTelemetry::Broadcast(Mode, msg);
    }
    break;
    case ezStats::StatsEventData::Remove:
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('STAT', ' DEL');
      msg.GetWriter() << e.m_szStatName;
      msg.GetWriter() << ezTime::Now();

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
    msg.SetMessageID('STAT', ' SET');
    msg.GetWriter() << it.Key().GetData();
    msg.GetWriter() << it.Value();

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
      ezTime FrameTime = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetFrameTime();

      ezStringBuilder s;
      ezStats::SetStat("App/FrameTime[ms]", FrameTime.GetMilliseconds());      
      ezStats::SetStat("App/FPS", 1.0 / FrameTime.GetSeconds());

      ezStats::SetStat("App/Active Threads", ezOSThread::GetThreadCount());

      // Tasksystem Thread Utilization
      {
        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks); ++t)
        {
          ezUInt32 uiNumTasks = 0;
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::ShortTasks, t, &uiNumTasks);

          s.Format("Utilization/Short{0}_Load[%%]", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), Utilization * 100.0);

          s.Format("Utilization/Short{0}_Tasks", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks); ++t)
        {
          ezUInt32 uiNumTasks = 0;
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::LongTasks, t, &uiNumTasks);

          s.Format("Utilization/Long{0}_Load[%%]", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), Utilization * 100.0);

          s.Format("Utilization/Long{0}_Tasks", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::FileAccess); ++t)
        {
          ezUInt32 uiNumTasks = 0;
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::FileAccess, t, &uiNumTasks);

          s.Format("Utilization/File{0}_Load[%%]", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), Utilization * 100.0);

          s.Format("Utilization/File{0}_Tasks", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), uiNumTasks);
        }
      }
    }
    break;

    default:
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
