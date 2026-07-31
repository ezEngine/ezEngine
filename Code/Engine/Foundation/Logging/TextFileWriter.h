#pragma once

#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/Mutex.h>

namespace ezLogWriter
{

  /// \brief A log writer that writes out log messages to a plain text file.
  ///
  /// Create an instance of this class, register the LogMessageHandler at ezLog and pass the pointer
  /// to the instance as the pPassThrough argument to it.
  ///
  /// In contrast to ezLogWriter::HTML this writes through ezOSFile, so it needs no data directories and
  /// can therefore already be active before the ezFileSystem is configured. Writes are unbuffered, so the
  /// log is complete on disk even if the application crashes. That makes this the log writer to use for
  /// automated tests and tools that are inspected by another process.
  class EZ_FOUNDATION_DLL TextFile
  {
  public:
    ~TextFile();

    /// \brief Register this at ezLog to write all log messages to a text file.
    void LogMessageHandler(const ezLoggingEventData& eventData);

    /// \brief Opens the given file (an absolute path) for writing the log. From now on all incoming log messages are written into it.
    ezResult BeginLog(ezStringView sFile);

    /// \brief Closes the file and stops logging the incoming messages.
    void EndLog();

    /// \brief Whether BeginLog() succeeded and EndLog() hasn't been called yet.
    bool IsOpen() const;

    /// \brief Allows to indicate in what form timestamps should be added to log messages.
    void SetTimestampMode(ezLog::TimestampMode mode);

  private:
    mutable ezMutex m_Mutex;
    ezOSFile m_File;
    ezLog::TimestampMode m_TimestampMode = ezLog::TimestampMode::None;
  };
} // namespace ezLogWriter
