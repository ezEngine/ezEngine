#pragma once

#include <Foundation/Logging/Log.h>

/// A simple log writer that writes out log messages using printf.
class EZ_FOUNDATION_DLL ezLog_ConsoleWriter
{
public:
  /// Register this at ezLog to write all log messages to the console using printf.
  static void LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough);

};