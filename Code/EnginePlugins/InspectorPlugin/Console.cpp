#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Core/Console/Console.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Configuration/CVar.h>

static void TelemetryMessage(void* pPassThrough)
{
  ezTelemetryMessage Msg;
  ezStringBuilder input;

  while (ezTelemetry::RetrieveMessage('CMD', Msg) == EZ_SUCCESS)
  {
    if (Msg.GetMessageID() == 'EXEC' || Msg.GetMessageID() == 'COMP')
    {
      Msg.GetReader() >> input;

      if (ezConsole::GetMainConsole())
      {
        if (auto pInt = ezConsole::GetMainConsole()->GetCommandInterpreter())
        {
          ezCommandInterpreterState s;
          s.m_sInput = input;

          ezStringBuilder encoded;

          if (Msg.GetMessageID() == 'EXEC')
          {
            pInt->Interpret(s);
          }
          else
          {
            pInt->AutoComplete(s);
            encoded.AppendFormat(";;00||<{}", s.m_sInput);
          }

          for (const auto& l : s.m_sOutput)
          {
            encoded.AppendFormat(";;{}||{}", ezArgI((ezInt32)l.m_Type, 2, true), l.m_sText);
          }

          ezTelemetryMessage msg;
          msg.SetMessageID('CMD', 'RES');
          msg.GetWriter() << encoded;
          ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
        }
      }
    }
  }
}

void AddConsoleEventHandler()
{
  ezTelemetry::AcceptMessagesForSystem('CMD', true, TelemetryMessage, nullptr);
}

void RemoveConsoleEventHandler()
{
  ezTelemetry::AcceptMessagesForSystem('CMD', false);
}
