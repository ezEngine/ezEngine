#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Strings/StringUtils.h>

/// \brief Use this helper macro to easily create a scoped logging group. Will generate unique variable names to make the static code analysis happy.
#define EZ_LOG_BLOCK ezLogBlock EZ_CONCAT(_logblock_, EZ_SOURCE_LINE)

// Forward declaration, class is at the end of this file
class ezLogBlock;


/// \brief Describes the types of events that ezLog sends.
struct EZ_FOUNDATION_DLL ezLogMsgType
{
  enum Enum
  {
    BeginGroup          = -2,   ///< A logging group has been opened.
    EndGroup            = -1,   ///< A logging group has been closed.
    None                = 0,    ///< Can be used to disable all log message types.
    ErrorMsg            = 1,    ///< An error message.
    SeriousWarningMsg   = 2,    ///< A serious warning message.
    WarningMsg          = 3,    ///< A warning message.
    SuccessMsg          = 4,    ///< A success message.
    InfoMsg             = 5,    ///< An info message.
    DevMsg              = 6,    ///< A development message.
    DebugMsg            = 7,    ///< A debug message.
    All                 = 8,    ///< Can be used to enable all log message types.
    ENUM_COUNT
  };
};

/// \brief The data that is sent through ezLogInterface.
struct EZ_FOUNDATION_DLL ezLoggingEventData
{
  ezLoggingEventData();

  /// \brief The type of information that is sent.
  ezLogMsgType::Enum m_EventType;

  /// \brief How many "levels" to indent.
  ezUInt32 m_uiIndentation;

  /// \brief The information text.
  const char* m_szText;

  /// \brief An optional tag extracted from the log-string (if it started with "[SomeTag]Logging String.") Can be used by log-writers for additional configuration, or simply be ignored.
  const char* m_szTag;
};

typedef ezEvent<const ezLoggingEventData&, ezMutex> ezLoggingEvent;

/// \brief Base class for all logging classes.
///
/// You can derive from this class to create your own logging system,
/// which you can pass to the functions in ezLog.
class EZ_FOUNDATION_DLL ezLogInterface
{
public:
  ezLogInterface() { m_pCurrentBlock = nullptr; }

  /// \brief Override this function to handle logging events.
  virtual void HandleLogMessage(const ezLoggingEventData& le) = 0;

private:
  friend class ezLog;
  friend class ezLogBlock;
  ezLogBlock* m_pCurrentBlock;
};

/// \brief This is the standard log system that ezLog sends all messages to.
///
/// It allows to register log writers, such that you can be informed of all log messages and write them
/// to different outputs.
class EZ_FOUNDATION_DLL ezGlobalLog : public ezLogInterface
{
public:
  static ezGlobalLog* GetInstance();

  virtual void HandleLogMessage(const ezLoggingEventData& le) override;

  /// \brief LogLevel is between ezLogEventType::None and ezLogEventType::All and defines which messages will be logged and which will be filtered out.
  static void SetLogLevel(ezLogMsgType::Enum LogLevel);

  /// \brief Returns the currently set log level.
  static ezLogMsgType::Enum GetLogLevel() { return s_LogLevel; }

  /// \brief Allows to register a function as an event receiver.
  static void AddLogWriter(ezLoggingEvent::Handler handler)    { s_LoggingEvent.AddEventHandler    (handler); }

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(ezLoggingEvent::Handler handler) { s_LoggingEvent.RemoveEventHandler (handler); }

  /// \brief Returns how many message of the given type occurred.
  static ezUInt32 GetMessageCount(ezLogMsgType::Enum MessageType) { return s_uiMessageCount[MessageType]; }

private:
  /// \brief Which messages to filter out.
  static ezLogMsgType::Enum s_LogLevel;

  /// \brief Counts the number of messages of each type.
  static ezAtomicInteger32 s_uiMessageCount[ezLogMsgType::ENUM_COUNT];

  /// \brief Manages all the Event Handlers for the logging events.
  static ezLoggingEvent s_LoggingEvent;

private:
  EZ_DISALLOW_COPY_AND_ASSIGN(ezGlobalLog);

  ezGlobalLog() { }

  static ezGlobalLog* s_pInstance;
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
  /// \brief Allows to change which logging system is used by default. If nothing is set, ezGlobalLog is used.
  static void SetDefaultLogSystem(ezLogInterface* pInterface);

  /// \brief Returns the currently set default logging system.
  static ezLogInterface* GetDefaultLogSystem();

  /// \brief An error that needs to be fixed as soon as possible.
  static void Error(const char* szFormat, ...);

  /// \brief Overload of Error() to output messages to a specific log.
  static void Error(ezLogInterface* pInterface, const char* szFormat, ...);

  /// \brief Not an error, but definitely a big problem, that should be looked into very soon.
  static void SeriousWarning(const char* szFormat, ...);

  /// \brief Overload of SeriousWarning() to output messages to a specific log.
  static void SeriousWarning(ezLogInterface* pInterface, const char* szFormat, ...);

  /// \brief A potential problem or a performance warning. Might be possible to ignore it.
  static void Warning(const char* szFormat, ...);

  /// \brief Overload of Warning() to output messages to a specific log.
  static void Warning(ezLogInterface* pInterface, const char* szFormat, ...);

  /// \brief Status information that something was completed successfully.
  static void Success(const char* szFormat, ...);

  /// \brief Overload of Success() to output messages to a specific log.
  static void Success(ezLogInterface* pInterface, const char* szFormat, ...);

  /// \brief Status information that is important.
  static void Info(const char* szFormat, ...);

  /// \brief Overload of Info() to output messages to a specific log.
  static void Info(ezLogInterface* pInterface, const char* szFormat, ...);

  /// \brief Status information that is nice to have during development.
  ///
  /// This function is compiled out in non-development builds.
  static void Dev(const char* szFormat, ...);

  /// \brief Overload of Dev() to output messages to a specific log.
  static void Dev(ezLogInterface* pInterface, const char* szFormat, ...);

  /// \brief Status information during debugging. Very verbose. Usually only temporarily added to the code.
  ///
  /// This function is compiled out in non-debug builds.
  static void Debug(const char* szFormat, ...);

  /// \brief Overload of Debug() to output messages to a specific log.
  static void Debug(ezLogInterface* pInterface, const char* szFormat, ...);

  /// \brief This is a placeholder function for verbose logging during debugging some piece of code.
  ///
  /// By default this function does not do anything (it has an empty body). It is meant to be redirected to one
  /// of the other logging methods (e.g. Debug() ), using a #define.
  /// This allows to only enable it in selected cpp files. E.g. you can use it in hundreds of source files, but
  /// the function will only have an effect in those few files where a\n
  /// \code{.cpp} #define VerboseDebugMessage Debug \endcode \n
  /// is inserted, which will actually redirect all logging to another function.
  /// You can commit your code that calls this logging function, but it should always be 'deactivated',
  /// e.g. the line \code{.cpp} #define VerboseDebugMessage Debug \endcode should always be commented out.
  static void VerboseDebugMessage(const char* szFormat, ...) { }

  /// \brief Overload of VerboseDebugMessage() to output messages to a specific log.
  static void VerboseDebugMessage(ezLogInterface* pInterface, const char* szFormat, ...) { }

private:
  // Needed to call 'EndLogBlock'
  friend class ezLogBlock;

  /// \brief The log system that messages are sent to when the user specifies no system himself.
  static ezLogInterface* s_DefaultLogSystem;

  /// \brief Ends grouping log messages.
  static void EndLogBlock(ezLogInterface* pInterface, ezLogBlock* pBlock);

  static void BroadcastLoggingEvent(ezLogInterface* pInterface, ezLogMsgType::Enum type, const char* szString);

  static void WriteBlockHeader(ezLogInterface* pInterface, ezLogBlock* pBlock);
};

/// \brief Insert this line of code into a cpp file to enable VerboseDebugMessage calls.
/// #define VerboseDebugMessage Debug


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
  ezInt32 m_iBlockDepth;
  ezLogBlock* m_pParentBlock;
  const char* m_szName;
  const char* m_szContextInfo;
  bool m_bWritten;
};

#include <Foundation/Logging/Implementation/Log_inl.h>

