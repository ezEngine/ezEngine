#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Strings/FormatString.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Threading/AtomicInteger.h>

/// \brief Use this helper macro to easily create a scoped logging group. Will generate unique variable names to make the static code
/// analysis happy.
#define EZ_LOG_BLOCK ezLogBlock EZ_CONCAT(_logblock_, EZ_SOURCE_LINE)

// Forward declaration, class is at the end of this file
class ezLogBlock;


/// \brief Describes the types of events that ezLog sends.
struct EZ_FOUNDATION_DLL ezLogMsgType
{
  typedef ezInt8 StorageType;

  enum Enum : ezInt8
  {
    BeginGroup = -2,       ///< A logging group has been opened.
    EndGroup = -1,         ///< A logging group has been closed.
    None = 0,              ///< Can be used to disable all log message types.
    ErrorMsg = 1,          ///< An error message.
    SeriousWarningMsg = 2, ///< A serious warning message.
    WarningMsg = 3,        ///< A warning message.
    SuccessMsg = 4,        ///< A success message.
    InfoMsg = 5,           ///< An info message.
    DevMsg = 6,            ///< A development message.
    DebugMsg = 7,          ///< A debug message.
    All = 8,               ///< Can be used to enable all log message types.
    ENUM_COUNT,
    Default = None,
  };
};

/// \brief The data that is sent through ezLogInterface.
struct EZ_FOUNDATION_DLL ezLoggingEventData
{
  /// \brief The type of information that is sent.
  ezLogMsgType::Enum m_EventType = ezLogMsgType::None;

  /// \brief How many "levels" to indent.
  ezUInt8 m_uiIndentation = 0;

  /// \brief The information text.
  const char* m_szText = "";

  /// \brief An optional tag extracted from the log-string (if it started with "[SomeTag]Logging String.") Can be used by log-writers for
  /// additional configuration, or simply be ignored.
  const char* m_szTag = "";

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  /// \brief Used by log-blocks for profiling the duration of the block
  double m_fSeconds = 0;
#endif
};

typedef ezEvent<const ezLoggingEventData&, ezMutex> ezLoggingEvent;

/// \brief Base class for all logging classes.
///
/// You can derive from this class to create your own logging system,
/// which you can pass to the functions in ezLog.
class EZ_FOUNDATION_DLL ezLogInterface
{
public:
  /// \brief Override this function to handle logging events.
  virtual void HandleLogMessage(const ezLoggingEventData& le) = 0;

  /// \brief LogLevel is between ezLogEventType::None and ezLogEventType::All and defines which messages will be logged and which will be
  /// filtered out.
  EZ_ALWAYS_INLINE void SetLogLevel(ezLogMsgType::Enum LogLevel) { m_LogLevel = LogLevel; }

  /// \brief Returns the currently set log level.
  EZ_ALWAYS_INLINE ezLogMsgType::Enum GetLogLevel() { return m_LogLevel; }

private:
  friend class ezLog;
  friend class ezLogBlock;
  ezLogBlock* m_pCurrentBlock = nullptr;
  ezLogMsgType::Enum m_LogLevel = ezLogMsgType::All;
};

/// \brief This is the standard log system that ezLog sends all messages to.
///
/// It allows to register log writers, such that you can be informed of all log messages and write them
/// to different outputs.
class EZ_FOUNDATION_DLL ezGlobalLog : public ezLogInterface
{
public:
  virtual void HandleLogMessage(const ezLoggingEventData& le) override;

  /// \brief Allows to register a function as an event receiver.
  static ezEventSubscriptionID AddLogWriter(ezLoggingEvent::Handler handler);

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(ezLoggingEvent::Handler handler);

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(ezEventSubscriptionID subscriptionID);

  /// \brief Returns how many message of the given type occurred.
  static ezUInt32 GetMessageCount(ezLogMsgType::Enum MessageType) { return s_uiMessageCount[MessageType]; }

private:
  /// \brief Counts the number of messages of each type.
  static ezAtomicInteger32 s_uiMessageCount[ezLogMsgType::ENUM_COUNT];

  /// \brief Manages all the Event Handlers for the logging events.
  static ezLoggingEvent s_LoggingEvent;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGlobalLog);

  friend class ezLog; // only ezLog may create instances of this class
  ezGlobalLog() {}
};

/// \brief Static class that allows to write out logging information.
///
/// This class takes logging information, prepares it and then broadcasts it to all interested code
/// via the event interface. It does not write anything on disk or somewhere else, itself. Instead it
/// allows to register custom log writers that can then write it to disk, to console, send it over a
/// network or pop up a message box. Whatever suits the current situation.
/// Since event handlers can be registered only temporarily, it is also possible to just gather all
/// errors that occur during some operation and then unregister the event handler again.
class EZ_FOUNDATION_DLL ezLog
{
public:
  /// \brief Allows to change which logging system is used by default on the current thread. If nothing is set, ezGlobalLog is used.
  ///
  /// Replacing the log system on a thread does not delete the previous system, so it can be reinstated later again.
  /// This can be used to temporarily route all logging to a custom system.
  static void SetThreadLocalLogSystem(ezLogInterface* pInterface);

  /// \brief Returns the currently set default logging system, or a thread local instance of ezGlobalLog, if nothing else was set.
  static ezLogInterface* GetThreadLocalLogSystem();

  /// \brief Sets the default log level which is used when new per thread logging systems are created on demand.
  ///
  /// Note that changes using this method will not affect already created log systems.
  static void SetDefaultLogLevel(ezLogMsgType::Enum LogLevel);

  /// \brief An error that needs to be fixed as soon as possible.
  static void Error(ezLogInterface* pInterface, const ezFormatString& string);

  /// \brief An error that needs to be fixed as soon as possible.
  template <typename... ARGS>
  static void Error(const char* szFormat, ARGS&&... args)
  {
    Error(GetThreadLocalLogSystem(), ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Error() to output messages to a specific log.
  template <typename... ARGS>
  static void Error(ezLogInterface* pInterface, const char* szFormat, ARGS&&... args)
  {
    Error(pInterface, ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Not an error, but definitely a big problem, that should be looked into very soon.
  static void SeriousWarning(ezLogInterface* pInterface, const ezFormatString& string);

  /// \brief Not an error, but definitely a big problem, that should be looked into very soon.
  template <typename... ARGS>
  static void SeriousWarning(const char* szFormat, ARGS&&... args)
  {
    SeriousWarning(GetThreadLocalLogSystem(), ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of SeriousWarning() to output messages to a specific log.
  template <typename... ARGS>
  static void SeriousWarning(ezLogInterface* pInterface, const char* szFormat, ARGS&&... args)
  {
    SeriousWarning(pInterface, ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief A potential problem or a performance warning. Might be possible to ignore it.
  static void Warning(ezLogInterface* pInterface, const ezFormatString& string);

  /// \brief A potential problem or a performance warning. Might be possible to ignore it.
  template <typename... ARGS>
  static void Warning(const char* szFormat, ARGS&&... args)
  {
    Warning(GetThreadLocalLogSystem(), ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Warning() to output messages to a specific log.
  template <typename... ARGS>
  static void Warning(ezLogInterface* pInterface, const char* szFormat, ARGS&&... args)
  {
    Warning(pInterface, ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information that something was completed successfully.
  static void Success(ezLogInterface* pInterface, const ezFormatString& string);

  /// \brief Status information that something was completed successfully.
  template <typename... ARGS>
  static void Success(const char* szFormat, ARGS&&... args)
  {
    Success(GetThreadLocalLogSystem(), ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Success() to output messages to a specific log.
  template <typename... ARGS>
  static void Success(ezLogInterface* pInterface, const char* szFormat, ARGS&&... args)
  {
    Success(pInterface, ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information that is important.
  static void Info(ezLogInterface* pInterface, const ezFormatString& string);

  /// \brief Status information that is important.
  template <typename... ARGS>
  static void Info(const char* szFormat, ARGS&&... args)
  {
    Info(GetThreadLocalLogSystem(), ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Info() to output messages to a specific log.
  template <typename... ARGS>
  static void Info(ezLogInterface* pInterface, const char* szFormat, ARGS&&... args)
  {
    Info(pInterface, ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  static void Dev(ezLogInterface* pInterface, const ezFormatString& string);

  /// \brief Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  template <typename... ARGS>
  static void Dev(const char* szFormat, ARGS&&... args)
  {
    Dev(GetThreadLocalLogSystem(), ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Dev() to output messages to a specific log.
  template <typename... ARGS>
  static void Dev(ezLogInterface* pInterface, const char* szFormat, ARGS&&... args)
  {
    Dev(pInterface, ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  static void Debug(ezLogInterface* pInterface, const ezFormatString& string);

  /// \brief Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  template <typename... ARGS>
  static void Debug(const char* szFormat, ARGS&&... args)
  {
    Debug(GetThreadLocalLogSystem(), ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Overload of Debug() to output messages to a specific log.
  template <typename... ARGS>
  static void Debug(ezLogInterface* pInterface, const char* szFormat, ARGS&&... args)
  {
    Debug(pInterface, ezFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
  }

  /// \brief Usually called internally by the other log functions, but can be called directly, if the message type is already known.
  /// pInterface must be != nullptr.
  static void BroadcastLoggingEvent(ezLogInterface* pInterface, ezLogMsgType::Enum type, const char* szString);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  /// \brief Calls low-level OS functionality to print a string to the typical outputs, e.g. printf and OutputDebugString.
  ///
  /// This function is meant for short term debugging when actual printing to the console is desired. Code using it should be temporary.
  /// \note This function uses actual printf formatting, not ezFormatString syntax.
  static void Printf(const char* szFormat, ...);
#endif

private:
  // Needed to call 'EndLogBlock'
  friend class ezLogBlock;

  /// \brief Which messages to filter out by default.
  static ezLogMsgType::Enum s_DefaultLogLevel;

  /// \brief Ends grouping log messages.
  static void EndLogBlock(ezLogInterface* pInterface, ezLogBlock* pBlock);

  static void WriteBlockHeader(ezLogInterface* pInterface, ezLogBlock* pBlock);
};


/// \brief Instances of this class will group messages in a scoped block together.
class EZ_FOUNDATION_DLL ezLogBlock
{
public:
  /// \brief Creates a named grouping block for log messages.
  ///
  /// Use the szContextInfo to pass in a string that can give additional context information (e.g. a file name).
  /// This string must point to valid memory until after the log block object is destroyed.
  /// Log writers get these strings provided through the ezLoggingEventData::m_szTag variable.
  /// \note The log block header (and context info) will not be printed until a message is successfully logged,
  /// i.e. as long as all messages in this block are filtered out (via the LogLevel setting), the log block
  /// header will not be printed, to prevent spamming the log.
  ///
  /// This constructor will output the log block data to the ezGlobalLog.
  ezLogBlock(const char* szName, const char* szContextInfo = "");

  /// \brief Creates a named grouping block for log messages.
  ///
  /// This variant of the constructor takes an explicit ezLogInterface to write the log messages to.
  ezLogBlock(ezLogInterface* pInterface, const char* szName, const char* szContextInfo = "");

  ~ezLogBlock();

private:
  friend class ezLog;

  ezLogInterface* m_pLogInterface;
  ezLogBlock* m_pParentBlock;
  const char* m_szName;
  const char* m_szContextInfo;
  ezUInt8 m_uiBlockDepth;
  bool m_bWritten;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  double m_fSeconds; // for profiling
#endif
};

/// \brief A class that sets a custom ezLogInterface as the thread local default log system,
/// and resets the previous system when it goes out of scope.
class EZ_FOUNDATION_DLL ezLogSystemScope
{
public:
  /// \brief The given ezLogInterface is passed to ezLog::SetThreadLocalLogSystem().
  explicit ezLogSystemScope(ezLogInterface* pInterface)
  {
    m_pPrevious = ezLog::GetThreadLocalLogSystem();
    ezLog::SetThreadLocalLogSystem(pInterface);
  }

  /// \brief Resets the previous ezLogInterface through ezLog::SetThreadLocalLogSystem()
  ~ezLogSystemScope() { ezLog::SetThreadLocalLogSystem(m_pPrevious); }

protected:
  ezLogInterface* m_pPrevious;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezLogSystemScope);
};

#include <Foundation/Logging/Implementation/Log_inl.h>
