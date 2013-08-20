#include <Foundation/PCH.h>
#include <Foundation/Logging/TelemetryWriter.h>
#include <Foundation/Communication/Telemetry.h>

namespace ezLogWriter
{
  void Telemetry::LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough)
  {
    ezTelemetryMessage msg;
    msg.SetMessageID('LOG', 'MSG');

    msg.GetWriter() << (ezInt16) EventData.m_EventType;
    msg.GetWriter() << (ezUInt16) EventData.m_uiIndentation;
    msg.GetWriter() << EventData.m_szTag;
    msg.GetWriter() << EventData.m_szText;

    ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
  }
}