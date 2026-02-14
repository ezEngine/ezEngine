#pragma once

#include <Foundation/Logging/Log.h>

namespace ezLogWriter
{
  /// \brief Log writer that forwards log messages as trace events.
  ///
  /// Emits each log message as a "LogMessage" trace event with fields Type, Indentation, and Text.
  class EZ_FOUNDATION_DLL Tracing
  {
  public:
    /// \brief Register this with ezGlobalLog::AddLogWriter to forward all log messages as trace events.
    static void LogMessageHandler(const ezLoggingEventData& eventData);
  };
} // namespace ezLogWriter
