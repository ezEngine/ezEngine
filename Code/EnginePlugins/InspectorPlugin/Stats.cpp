#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/GameApplication/GameApplicationBase.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Utilities/Stats.h>

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
      msg.GetWriter() << e.m_sStatName;
      msg.GetWriter() << e.m_NewStatValue;
      msg.GetWriter() << ezTime::Now();

      ezTelemetry::Broadcast(Mode, msg);
    }
    break;
    case ezStats::StatsEventData::Remove:
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('STAT', ' DEL');
      msg.GetWriter() << e.m_sStatName;
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

    default:
      break;
  }
}

static void PerFrameUpdateHandler(const ezGameApplicationExecutionEvent& e)
{
  switch (e.m_Type)
  {
    case ezGameApplicationExecutionEvent::Type::AfterPresent:
    {
      ezTime FrameTime;

      if (ezGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
      {
        FrameTime = ezGameApplicationBase::GetGameApplicationBaseInstance()->GetFrameTime();
      }

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

          s.SetFormat("Utilization/Short{0}_Load[%%]", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Short{0}_Tasks", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::LongTasks); ++t)
        {
          ezUInt32 uiNumTasks = 0;
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::LongTasks, t, &uiNumTasks);

          s.SetFormat("Utilization/Long{0}_Load[%%]", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/Long{0}_Tasks", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), uiNumTasks);
        }

        for (ezUInt32 t = 0; t < ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::FileAccess); ++t)
        {
          ezUInt32 uiNumTasks = 0;
          const double Utilization = ezTaskSystem::GetThreadUtilization(ezWorkerThreadType::FileAccess, t, &uiNumTasks);

          s.SetFormat("Utilization/File{0}_Load[%%]", ezArgI(t, 2, true));
          ezStats::SetStat(s.GetData(), Utilization * 100.0);

          s.SetFormat("Utilization/File{0}_Tasks", ezArgI(t, 2, true));
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

  // We're handling the per frame update by a different event since
  // using ezTelemetry::TelemetryEventData::PerFrameUpdate can lead
  // to deadlocks between the ezStats and ezTelemetry system.
  if (ezGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.AddEventHandler(PerFrameUpdateHandler);
  }
}

void RemoveStatsEventHandler()
{
  if (ezGameApplicationBase::GetGameApplicationBaseInstance() != nullptr)
  {
    ezGameApplicationBase::GetGameApplicationBaseInstance()->m_ExecutionEvents.RemoveEventHandler(PerFrameUpdateHandler);
  }

  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);

  ezStats::RemoveEventHandler(StatsEventHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Stats);
