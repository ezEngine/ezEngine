#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Reflection/Reflection.h>

namespace ReflectionDetail
{

  static void SendBasicTypesGroup()
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << "Basic Types";
    msg.GetWriter() << "";
    msg.GetWriter() << 0;
    msg.GetWriter() << "";
    msg.GetWriter() << (ezUInt32)0U;
    msg.GetWriter() << (ezUInt32)0U;

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }

  static ezStringView GetParentType(const ezRTTI* pRTTI)
  {
    if (pRTTI->GetParentType())
    {
      return pRTTI->GetParentType()->GetTypeName();
    }

    if ((pRTTI->GetTypeName() == "bool") || (pRTTI->GetTypeName() == "float") ||
        (pRTTI->GetTypeName() == "double") || (pRTTI->GetTypeName() == "ezInt8") ||
        (pRTTI->GetTypeName() == "ezUInt8") || (pRTTI->GetTypeName() == "ezInt16") ||
        (pRTTI->GetTypeName() == "ezUInt16") || (pRTTI->GetTypeName() == "ezInt32") ||
        (pRTTI->GetTypeName() == "ezUInt32") || (pRTTI->GetTypeName() == "ezInt64") ||
        (pRTTI->GetTypeName() == "ezUInt64") || (pRTTI->GetTypeName() == "ezConstCharPtr") ||
        (pRTTI->GetTypeName() == "ezVec2") || (pRTTI->GetTypeName() == "ezVec3") ||
        (pRTTI->GetTypeName() == "ezVec4") || (pRTTI->GetTypeName() == "ezMat3") ||
        (pRTTI->GetTypeName() == "ezMat4") || (pRTTI->GetTypeName() == "ezTime") ||
        (pRTTI->GetTypeName() == "ezUuid") || (pRTTI->GetTypeName() == "ezColor") ||
        (pRTTI->GetTypeName() == "ezVariant") || (pRTTI->GetTypeName() == "ezQuat"))
    {
      return "Basic Types";
    }

    return {};
  }

  static void SendReflectionTelemetry(const ezRTTI* pRTTI)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('RFLC', 'DATA');
    msg.GetWriter() << pRTTI->GetTypeName();
    msg.GetWriter() << GetParentType(pRTTI);
    msg.GetWriter() << pRTTI->GetTypeSize();
    msg.GetWriter() << pRTTI->GetPluginName();

    {
      auto properties = pRTTI->GetProperties();

      msg.GetWriter() << properties.GetCount();

      for (auto& prop : properties)
      {
        msg.GetWriter() << prop->GetPropertyName();
        msg.GetWriter() << (ezInt8)prop->GetCategory();

        const ezRTTI* pType = prop->GetSpecificType();
        msg.GetWriter() << (pType ? pType->GetTypeName() : "<Unknown Type>");
      }
    }

    {
      const ezArrayPtr<ezAbstractMessageHandler*>& Messages = pRTTI->GetMessageHandlers();

      msg.GetWriter() << Messages.GetCount();

      for (ezUInt32 i = 0; i < Messages.GetCount(); ++i)
      {
        msg.GetWriter() << Messages[i]->GetMessageId();
      }
    }

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }

  static void SendAllReflectionTelemetry()
  {
    if (!ezTelemetry::IsConnectedToClient())
      return;

    // clear
    {
      ezTelemetryMessage msg;
      ezTelemetry::Broadcast(ezTelemetry::Reliable, 'RFLC', ' CLR', nullptr, 0);
    }

    SendBasicTypesGroup();

    ezRTTI::ForEachType([](const ezRTTI* pRtti)
      { SendReflectionTelemetry(pRtti); });
  }


  static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case ezTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllReflectionTelemetry();
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
        SendAllReflectionTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace ReflectionDetail

void AddReflectionEventHandler()
{
  ezTelemetry::AddEventHandler(ReflectionDetail::TelemetryEventsHandler);

  ezPlugin::Events().AddEventHandler(ReflectionDetail::PluginEventHandler);
}

void RemoveReflectionEventHandler()
{
  ezPlugin::Events().RemoveEventHandler(ReflectionDetail::PluginEventHandler);

  ezTelemetry::RemoveEventHandler(ReflectionDetail::TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Reflection);
