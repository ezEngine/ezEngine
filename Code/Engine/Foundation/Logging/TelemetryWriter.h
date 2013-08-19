#pragma once

#include <Foundation/Logging/Log.h>

namespace ezLogWriter
{
  /// \brief This logwriter will broadcast all messages through ezTelemetry, such that external applications can display the log messages.
  class EZ_FOUNDATION_DLL Telemetry
  {
  public:
    /// \brief Register this at ezLog to broadcast all log messages through ezTelemetry.
    static void LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough);

  };
}