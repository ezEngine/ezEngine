#pragma once

#include <Foundation/Logging/Log.h>

/// \brief A simple log writer that outputs all log messages to the ez ETW provider.
class EZ_FOUNDATION_DLL ezETW
{
public:
  /// \brief Register this at ezLog to write all log messages to ETW.
  static void LogMessageHandler(const ezLoggingEventData& eventData)
  {
    ezETW::LogMessage(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_sText);
  }

  /// \brief Log Message to ETW.
  static void LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText);
};
