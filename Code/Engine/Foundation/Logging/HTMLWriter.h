#pragma once

#include <Foundation/Logging/Log.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

/// A log writer that writes out log messages to an HTML file.
/// Create an instance of this class, register the LogMessageHandler at ezLog and pass the pointer 
/// to the instance as the pPassThrough argument to it.
class EZ_FOUNDATION_DLL ezLog_HTMLWriter
{
public:
  ~ezLog_HTMLWriter ();

  /// Register this at ezLog to write all log messages to an HTML file.
  static void LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough);

  /// Opens the given file for writing the log. From now on all incoming log messages are written into it.
  void BeginLog(const char* szFile, const char* szAppTitle);

  /// Closes the HTML file and stops logging the incoming message.
  void EndLog ();

  /// Returns the name of the log-file that was really opened. Might be slightly different than what was given to BeginLog, to allow parallel execution of the same application.
  const ezFileWriter& GetOpenedLogFile() const;

private:
  void MessageHandler(const ezLog::LoggingEvent& EventData);

  void WriteString(const char* szString, ezUInt32 uiColor);

  ezFileWriter m_File;
};

