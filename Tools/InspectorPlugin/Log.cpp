#include <PCH.h>
#include <Foundation/Communication/Telemetry.h>
#include <Foundation/Logging/Log.h>

namespace ezLogWriter
{
  /// \brief This log-writer will broadcast all messages through ezTelemetry, such that external applications can display the log messages.
  class Telemetry
  {
  public:
    /// \brief Register this at ezLog to broadcast all log messages through ezTelemetry.
    static void LogMessageHandler(const ezLoggingEventData& eventData)
    {
      ezTelemetryMessage msg;
      msg.SetMessageID('LOG', 'MSG');

      msg.GetWriter() << (ezInt16) eventData.m_EventType;
      msg.GetWriter() << (ezUInt16) eventData.m_uiIndentation;
      msg.GetWriter() << eventData.m_szTag;
      msg.GetWriter() << eventData.m_szText;

      ezTelemetry::Broadcast(ezTelemetry::Reliable, msg);
    }
  };
}

void AddLogWriter()
{
  ezGlobalLog::AddLogWriter(&ezLogWriter::Telemetry::LogMessageHandler);
}

void RemoveLogWriter()
{
  ezGlobalLog::RemoveLogWriter(&ezLogWriter::Telemetry::LogMessageHandler);
}



EZ_STATICLINK_FILE(InspectorPlugin, InspectorPlugin_Log);

