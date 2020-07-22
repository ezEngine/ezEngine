#pragma once

#include <Foundation/Logging/Log.h>

namespace ezLogWriter
{
  /// \brief A simple log writer that writes out log messages using printf.
  class EZ_FOUNDATION_DLL Console
  {
  public:
    /// \brief Register this at ezLog to write all log messages to the console using printf.
    static void LogMessageHandler(const ezLoggingEventData& eventData);

    /// \brief Allows to indicate in what form timestamps should be added to log messages.
    static void SetTimestampMode(ezLog::TimestampMode mode);

  private:
    static ezLog::TimestampMode s_TimestampMode;
  };
} // namespace ezLogWriter
