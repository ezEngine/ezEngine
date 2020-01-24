#include <InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  ezTelemetryMessage Msg;

  while (ezTelemetry::RetrieveMessage('SVAR', Msg) == EZ_SUCCESS)
  {
    if (Msg.GetMessageID() == ' SET')
    {
      ezString sCVar;
      ezUInt8 uiType;

      float fValue;
      ezInt32 iValue;
      bool bValue;
      ezString sValue;

      Msg.GetReader() >> sCVar;
      Msg.GetReader() >> uiType;

      switch (uiType)
      {
        case ezCVarType::Float:
          Msg.GetReader() >> fValue;
          break;
        case ezCVarType::Int:
          Msg.GetReader() >> iValue;
          break;
        case ezCVarType::Bool:
          Msg.GetReader() >> bValue;
          break;
        case ezCVarType::String:
          Msg.GetReader() >> sValue;
          break;
      }

      ezCVar* pCVar = ezCVar::GetFirstInstance();

      while (pCVar)
      {
        if (((ezUInt8)pCVar->GetType() == uiType) && (pCVar->GetName() == sCVar))
        {
          switch (uiType)
          {
            case ezCVarType::Float:
              *((ezCVarFloat*)pCVar) = fValue;
              break;
            case ezCVarType::Int:
              *((ezCVarInt*)pCVar) = iValue;
              break;
            case ezCVarType::Bool:
              *((ezCVarBool*)pCVar) = bValue;
              break;
            case ezCVarType::String:
              *((ezCVarString*)pCVar) = sValue;
              break;
          }
        }

        pCVar = pCVar->GetNextInstance();
      }
    }
  }
}

static void SendCVarTelemetry(ezCVar* pCVar)
{
  ezTelemetryMessage msg;
  msg.SetMessageID('CVAR', 'DATA');
  msg.GetWriter() << pCVar->GetName();
  msg.GetWriter() << pCVar->GetPluginName();
  // msg.GetWriter() << (ezUInt8) pCVar->GetFlags().GetValue(); // currently not used
  msg.GetWriter() << (ezUInt8)pCVar->GetType();
  msg.GetWriter() << pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case ezCVarType::Float:
    {
      const float val = ((ezCVarFloat*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case ezCVarType::Int:
    {
      const int val = ((ezCVarInt*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case ezCVarType::Bool:
    {
      const bool val = ((ezCVarBool*)pCVar)->GetValue();
      msg.GetWriter() << val;
    }
    break;
    case ezCVarType::String:
    {
      const char* val = ((ezCVarString*)pCVar)->GetValue().GetData();
      msg.GetWriter() << val;
    }
    break;

    case ezCVarType::ENUM_COUNT:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
}

static void SendAllCVarTelemetry()
{
  if (!ezTelemetry::IsConnectedToClient())
    return;

  // clear
  {
    ezTelemetryMessage msg;
    ezTelemetry::Broadcast(ezTelemetry::Reliable, 'CVAR', ' CLR', nullptr, 0);
  }

  ezCVar* pCVar = ezCVar::GetFirstInstance();

  while (pCVar)
  {
    SendCVarTelemetry(pCVar);

    pCVar = pCVar->GetNextInstance();
  }

  {
    ezTelemetryMessage msg;
    ezTelemetry::Broadcast(ezTelemetry::Reliable, 'CVAR', 'SYNC', nullptr, 0);
  }
}

namespace CVarsDetail
{

  static void TelemetryEventsHandler(const ezTelemetry::TelemetryEventData& e)
  {
    switch (e.m_EventType)
    {
      case ezTelemetry::TelemetryEventData::ConnectedToClient:
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }

  static void CVarEventHandler(const ezCVarEvent& e)
  {
    if (!ezTelemetry::IsConnectedToClient())
      return;

    switch (e.m_EventType)
    {
      case ezCVarEvent::ValueChanged:
        SendCVarTelemetry(e.m_pCVar);
        break;

      case ezCVarEvent::ListOfVarsChanged:
        SendAllCVarTelemetry();
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
        SendAllCVarTelemetry();
        break;

      default:
        break;
    }
  }
} // namespace CVarsDetail

void AddCVarEventHandler()
{
  ezTelemetry::AddEventHandler(CVarsDetail::TelemetryEventsHandler);
  ezTelemetry::AcceptMessagesForSystem('SVAR', true, TelemetryMessage, nullptr);

  ezCVar::s_AllCVarEvents.AddEventHandler(CVarsDetail::CVarEventHandler);
  ezPlugin::s_PluginEvents.AddEventHandler(CVarsDetail::PluginEventHandler);
}

void RemoveCVarEventHandler()
{
  ezPlugin::s_PluginEvents.RemoveEventHandler(CVarsDetail::PluginEventHandler);
  ezCVar::s_AllCVarEvents.RemoveEventHandler(CVarsDetail::CVarEventHandler);

  ezTelemetry::RemoveEventHandler(CVarsDetail::TelemetryEventsHandler);
  ezTelemetry::AcceptMessagesForSystem('SVAR', false);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_CVars);
