#pragma once

#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>

namespace ezLogWriter
{

  /// \brief A log writer that writes out log messages to an HTML file.
  ///
  /// Create an instance of this class, register the LogMessageHandler at ezLog and pass the pointer
  /// to the instance as the pPassThrough argument to it.
  class EZ_FOUNDATION_DLL HTML
  {
  public:
    ~HTML();

    /// \brief Register this at ezLog to write all log messages to an HTML file.
    void LogMessageHandler(const ezLoggingEventData& eventData);

    /// \brief Opens the given file for writing the log. From now on all incoming log messages are written into it.
    void BeginLog(ezStringView sFile, ezStringView sAppTitle);

    /// \brief Closes the HTML file and stops logging the incoming message.
    void EndLog();

    /// \brief Returns the name of the log-file that was really opened. Might be slightly different than what was given to BeginLog, to allow parallel
    /// execution of the same application.
    const ezFileWriter& GetOpenedLogFile() const;

    /// \brief Allows to indicate in what form timestamps should be added to log messages.
    void SetTimestampMode(ezLog::TimestampMode mode);

  private:
    void WriteString(ezStringView sText, ezUInt32 uiColor);

    ezFileWriter m_File;

    ezLog::TimestampMode m_TimestampMode = ezLog::TimestampMode::None;
  };
} // namespace ezLogWriter
