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

          if (Msg.GetMessageID() == 'EXEC')
            pInt->Interpret(s);
          else
            pInt->AutoComplete(s);

          ezTelemetryMessage msg;
          msg.SetMessageID('CMD', 'RES');
          msg.GetWriter() << s.m_sInput;
          msg.GetWriter() << (ezUInt16)s.m_sOutput.GetCount();
          for (const auto& l : s.m_sOutput)
          {
            msg.GetWriter() << l.m_sText;
            msg.GetWriter() << (ezInt16)l.m_Type;
          }
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
