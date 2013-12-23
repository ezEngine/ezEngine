#pragma once

#include <Foundation/Logging/Log.h>

namespace ezLogWriter
{

  /// \brief A simple log writer that outputs all log messages to visual studios output window
  class EZ_FOUNDATION_DLL VisualStudio
  {
  public:
    /// \brief Register this at ezLog to write all log messages to visual studios output window.
    static void LogMessageHandler(const ezLoggingEventData& eventData);

  };

}

