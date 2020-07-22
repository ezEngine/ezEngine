#pragma once

#include <Foundation/Logging/Log.h>

namespace ezLogWriter
{

  /// \brief A simple log writer that outputs all log messages to the ez ETW provider.
  class EZ_FOUNDATION_DLL ETW
  {
  public:
    /// \brief Register this at ezLog to write all log messages to ETW.
    static void LogMessageHandler(const ezLoggingEventData& eventData);

    /// \brief Log Message to ETW.
    static void LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, const char* szText);
  };
} // namespace ezLogWriter
