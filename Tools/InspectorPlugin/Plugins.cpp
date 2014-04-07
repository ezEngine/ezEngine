#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>

static void SendPluginTelemetry()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, 'PLUG', 'CLR', nullptr, 0);

  ezPlugin* pSub = ezPlugin::GetFirstInstance();

  while (pSub)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('PLUG', 'DATA');
    msg.GetWriter() << pSub->GetPluginName();
    msg.GetWriter() << pSub->IsReloadable();

    ezStringBuilder s;

    ezInt32 iDep = 0;
    while (pSub->GetPluginDependency(iDep) != nullptr)
    {
      if (!s.IsEmpty())
        s.Append(" | ");

      s.Append(pSub->GetPluginDependency(iDep));

      ++iDep;
    }

    msg.GetWriter() << s.GetData();

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);

    pSub = pSub->GetNextInstance();
  }
}

static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendPluginTelemetry();
    break;
  }
}

static void PluginEventHandler(const ezPlugin::PluginEvent& e)
{
  switch (e.m_EventType)
  {
  case ezPlugin::PluginEvent::AfterPluginChanges:
    SendPluginTelemetry();
    break;
  }
}

void AddPluginEventHandler()
{
  ezTelemetry::AddEventHandler(TelemetryEventsHandler);
  ezPlugin::s_PluginEvents.AddEventHandler(PluginEventHandler);
}

void RemovePluginEventHandler()
{
  ezPlugin::s_PluginEvents.RemoveEventHandler(PluginEventHandler);
  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Plugins);

