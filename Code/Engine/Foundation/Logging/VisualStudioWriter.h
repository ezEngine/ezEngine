#pragma once

#include <Foundation/Logging/Log.h>

/// A simple log writer that outputs all log messages to visual studios output window
class EZ_FOUNDATION_DLL ezLog_VisualStudioWriter
{
public:
  /// Register this at ezLog to write all log messages to visual studios output window.
  static void LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough);

};