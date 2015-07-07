#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Reflection/Reflection.h>

static void SendBasicTypesGroup()
{
  ezTelemetryMessage msg;
  msg.SetMessageID('RFLC', 'DATA');
  msg.GetWriter() << "Basic Types";
  msg.GetWriter() << "";
  msg.GetWriter() << 0;
  msg.GetWriter() << "";
  msg.GetWriter() << (ezUInt32) 0U;
  msg.GetWriter() << (ezUInt32) 0U;

  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
}

const char* GetParentType(ezRTTI* pRTTI)
{
  if (pRTTI->GetParentType())
    return pRTTI->GetParentType()->GetTypeName();

  if ((ezStringUtils::IsEqual(pRTTI->GetTypeName(), "bool")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "float")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "double")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezInt8")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezUInt8")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezInt16")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezUInt16")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezInt32")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezUInt32")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezInt64")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezUInt64")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezConstCharPtr")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezVec2")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezVec3")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezVec4")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezMat3")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezMat4")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezTime")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezUuid")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezColor")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezVariant")) ||
      (ezStringUtils::IsEqual(pRTTI->GetTypeName(), "ezQuat")))
    return "Basic Types";

  return "";
}

static void SendReflectionTelemetry(ezRTTI* pRTTI)
{
  ezTelemetryMessage msg;
  msg.SetMessageID('RFLC', 'DATA');
  msg.GetWriter() << pRTTI->GetTypeName();
  msg.GetWriter() << GetParentType(pRTTI);
  msg.GetWriter() << pRTTI->GetTypeSize();
  msg.GetWriter() << pRTTI->GetPluginName();

  {
    const ezArrayPtr<ezAbstractProperty*>& Properties = pRTTI->GetProperties();

    msg.GetWriter() << Properties.GetCount();

    for (ezUInt32 i = 0; i < Properties.GetCount(); ++i)
    {
      msg.GetWriter() << Properties[i]->GetPropertyName();
      msg.GetWriter() << (ezInt8) Properties[i]->GetCategory();
      msg.GetWriter() << Properties[i]->GetSpecificType()->GetTypeName();
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
    ezTelemetry::Broadcast(ezTelemetry::Reliable, 'RFLC', 'CLR', nullptr, 0);
  }

  SendBasicTypesGroup();

  ezRTTI* pRTTI = ezRTTI::GetFirstInstance();

  while (pRTTI)
  {
    SendReflectionTelemetry(pRTTI);

    pRTTI = pRTTI->GetNextInstance();
  }
}


static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
{
  switch (e.m_EventType)
  {
  case ezTelemetry::TelemetryEventData::ConnectedToClient:
    SendAllReflectionTelemetry();
    break;
  }
}

static void PluginEventHandler(const ezPlugin::PluginEvent& e)
{
  switch (e.m_EventType)
  {
  case ezPlugin::PluginEvent::AfterPluginChanges:
    SendAllReflectionTelemetry();
    break;
  }
}


void AddReflectionEventHandler()
{
  ezTelemetry::AddEventHandler(TelemetryEventsHandler);

  ezPlugin::s_PluginEvents.AddEventHandler(PluginEventHandler);
}

void RemoveReflectionEventHandler()
{
  ezPlugin::s_PluginEvents.RemoveEventHandler(PluginEventHandler);

  ezTelemetry::RemoveEventHandler(TelemetryEventsHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Reflection);

