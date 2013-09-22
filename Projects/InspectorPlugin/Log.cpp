#include <PCH.h>

namespace ezLogWriter
{
  /// \brief This logwriter will broadcast all messages through ezTelemetry, such that external applications can display the log messages.
  class Telemetry
  {
  public:
    /// \brief Register this at ezLog to broadcast all log messages through ezTelemetry.
    static void LogMessageHandler(const ezLog::EventData& eventData)
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
  ezLog::AddLogWriter(&ezLogWriter::Telemetry::LogMessageHandler);
}

void RemoveLogWriter()
{
  ezLog::RemoveLogWriter(&ezLogWriter::Telemetry::LogMessageHandler);
}

