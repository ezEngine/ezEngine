#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/Plugin.h>

namespace PluginsDetail
{
  static void SendPluginTelemetry()
  {
    if (!ezTelemetry::IsConnectedToClient())
      return;

    ezTelemetry::Broadcast(ezTelemetry::Reliable, 'PLUG', ' CLR', nullptr, 0);

    ezHybridArray<ezPlugin::PluginInfo, 16> infos;
    ezPlugin::GetAllPluginInfos(infos);

    for (const auto& pi : infos)
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('PLUG', 'DATA');
      msg.GetWriter() << pi.m_sName;
      msg.GetWriter() << false; // deprecated 'IsReloadable' flag

      ezStringBuilder s;

      for (const auto& dep : pi.m_sDependencies)
      {
        s.AppendWithSeparator(" | ", dep);
      }

      msg.GetWriter() << s;

      ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
    }
  }

  static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case ezTelemetry::TelemetryEventData::ConnectedToClient:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }

  static void PluginEventHandler(const ezPluginEvent& e)
  {
    switch (e.m_EventType)
    {
      case ezPluginEvent::AfterPluginChanges:
        SendPluginTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace PluginsDetail

void AddPluginEventHandler()
{
  ezTelemetry::AddEventHandler(PluginsDetail::TelemetryEventsHandler);
  ezPlugin::Events().AddEventHandler(PluginsDetail::PluginEventHandler);
}

void RemovePluginEventHandler()
{
  ezPlugin::Events().RemoveEventHandler(PluginsDetail::PluginEventHandler);
  ezTelemetry::RemoveEventHandler(PluginsDetail::TelemetryEventsHandler);
}


