#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Communication/GlobalEvent.h>
#include <Foundation/Configuration/Startup.h>

static void SendSubsystemTelemetry();
static ezInt32 s_iSendSubSystemTelemetry = 0;

EZ_ON_GLOBAL_EVENT(ezStartup_StartupCore_End)
{
  SendSubsystemTelemetry();
}

EZ_ON_GLOBAL_EVENT(ezStartup_StartupEngine_End)
{
  SendSubsystemTelemetry();
}

EZ_ON_GLOBAL_EVENT(ezStartup_ShutdownCore_End)
{
  SendSubsystemTelemetry();
}

EZ_ON_GLOBAL_EVENT(ezStartup_ShutdownEngine_End)
{
  SendSubsystemTelemetry();
}

static void SendSubsystemTelemetry()
{
  if (s_iSendSubSystemTelemetry <= 0)
    return;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, 'STRT', 'CLR', nullptr, 0);

  ezSubSystem* pSub = ezSubSystem::GetFirstInstance();

  while (pSub)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('STRT', 'SYST');
    msg.GetWriter() << pSub->GetGroupName();
    msg.GetWriter() << pSub->GetSubSystemName();
    msg.GetWriter() << pSub->GetPluginName();

    for (ezUInt32 i = 0; i < ezStartupStage::ENUM_COUNT; ++i)
      msg.GetWriter() << pSub->IsStartupPhaseDone((ezStartupStage::Enum) i);

    ezUInt8 uiDependencies = 0;
    while (pSub->GetDependency(uiDependencies) != nullptr)
      ++uiDependencies;

    msg.GetWriter() << uiDependencies;

    for (ezUInt8 i = 0; i < uiDependencies; ++i)
      msg.GetWriter() << pSub->GetDependency(i);

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);

    pSub = pSub->GetNextInstance();
  }
}

static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendSubsystemTelemetry();
    break;
  }
}


void AddStartupEventHandler()
{
  ++s_iSendSubSystemTelemetry;
  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
}

void RemoveStartupEventHandler()
{
  --s_iSendSubSystemTelemetry;
  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Startup);

