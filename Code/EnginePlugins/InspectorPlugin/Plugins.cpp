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

    ezPlugin* pSub = ezPlugin::GetFirstInstance();

    while (pSub)
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('PLUG', 'DATA');
      msg.GetWriter() << pSub->GetPluginName();
      msg.GetWriter() << pSub->IsReloadable();

      ezStringBuilder s;

      ezUInt8 uiDep = 0;
      while (pSub->GetPluginDependency(uiDep) != nullptr)
      {
        if (!s.IsEmpty())
          s.Append(" | ");

        s.Append(pSub->GetPluginDependency(uiDep));

        ++uiDep;
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
  ezPlugin::s_PluginEvents.AddEventHandler(PluginsDetail::PluginEventHandler);
}

void RemovePluginEventHandler()
{
  ezPlugin::s_PluginEvents.RemoveEventHandler(PluginsDetail::PluginEventHandler);
  ezTelemetry::RemoveEventHandler(PluginsDetail::TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Plugins);
