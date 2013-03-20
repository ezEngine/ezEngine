#pragma once

#include <Foundation/Communication/Event.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/ThreadLocalPointer.h>
#include <Foundation/Strings/StringUtils.h>

/// \brief Use this helper macro to easily create a scoped logging group. Will generate unique variable names to make the static code analysis happy.
#define EZ_LOG_BLOCK(name) ezLogBlock EZ_CONCAT(_logblock_, EZ_SOURCE_LINE)(name);

// Forward declaration, class is at the end of this file
class ezLogBlock;

/// \brief Static class that allows to write out logging information.
/// This class takes logging information, prepares it and then broadcasts it to all interested code
/// via the event interface. It does not write anything on disk or somewhere else, itself. Instead it
/// allows to register custom log writers that can then write it to disk, to console, send it over a
/// network or pop up a message box. Whatever suits the current situation.
/// Since event handlers can be registered only temporarily, it is also possible to just gather all
/// errors that occur during some operation and then unregister the event handler again.
class EZ_FOUNDATION_DLL ezLog
{
public:
  /// Describes the types of events that ezLog sends.
  struct EventType
  {
    enum Enum
    {
      FlushToDisk         = -3,
      BeginGroup          = -2,
      EndGroup            = -1,
      None                = 0,
      FatalErrorMsg       = 1,
      ErrorMsg            = 2,
      SeriousWarningMsg   = 3,
      WarningMsg          = 4,
      SuccessMsg          = 5,
      InfoMsg             = 6,
      DevMsg              = 7,
      DebugMsg            = 8,
      DebugRegularMsg     = 9,
      All                 = 9,

      ENUM_COUNT
    };
  };

  /// \brief The data that is sent through the event interface.
  struct LoggingEvent
  {
    LoggingEvent();

    /// \brief The type of information that is sent.
    EventType::Enum m_EventType;

    /// \brief How many "levels" to indent.
    ezUInt32 m_uiIndentation;

    /// \brief The information text.
    const char* m_szText;

    /// \brief An optional tag extracted from the log-string (if it started with "[SomeTag]Logging String.") Can be used by log-writers for additional configuration, or simply be ignored.
    const char* m_szTag;
  };

public:

  /// \brief LogLevel is between ezLogEventType::None and ezLogEventType::All and defines which messages will be logged and which will be filtered out.
  static void SetLogLevel(EventType::Enum LogLevel);

  /// \brief An error that results in program termination.
  static void FatalError(const char* szFormat, ...);

  /// \brief An error that needs to be fixed as soon as possible.
  static void Error(const char* szFormat, ...);

  /// \brief Not an error, but definitely a big problem, that should be looked into very soon.
  static void SeriousWarning(const char* szFormat, ...);

  /// \brief A potential problem or a performance warning. Might be possible to ignore it.
  static void Warning(const char* szFormat, ...);

  /// \brief Status information that something was completed successfully.
  static void Success(const char* szFormat, ...);

  /// \brief Status information that is important.
  static void Info(const char* szFormat, ...);

  /// \brief Status information that is nice to have during development.
  /// This function is compiled out in non-development builds.
  static void Dev(const char* szFormat, ...);

  /// \brief Status information during debugging. Very verbose. Usually only temporarily added to the code.
  /// This function is compiled out in non-debug builds.
  static void Debug(const char* szFormat, ...);

  /// \brief Status information during debugging. Very verbose, happening every frame. Usually only temporarily added to the code.
  /// This function is compiled out in non-debug builds.
  static void DebugRegular(const char* szFormat, ...);

  /// \brief Can be called at any time to make sure the log is written to disk.
  static void FlushToDisk();

  /// \brief Allows to register a function as an event receiver. All receivers will be notified in the order that they registered.
  static void AddLogWriter(ezEvent<const LoggingEvent&>::ezEventHandler callback, void* pPassThrough = NULL)   { s_LoggingEvent.AddEventHandler   (callback, pPassThrough); }

  /// \brief Unregisters a previously registered receiver. It is an error to unregister a receiver that was not registered.
  static void RemoveLogWriter(ezEvent<const LoggingEvent&>::ezEventHandler callback, void* pPassThrough = NULL) { s_LoggingEvent.RemoveEventHandler (callback, pPassThrough); }

  /// \brief Returns how many message of the given type occured.
  static ezUInt32 GetMessageCount(EventType::Enum MessageType) { return s_uiMessageCount[MessageType]; }

private:
  // Needed to call 'EndLogBlock'
  friend class ezLogBlock;

  /// \brief Ends grouping log messages.
  static void EndLogBlock(ezLogBlock* pBlock);

  /// \brief Which messages to filter out.
  static EventType::Enum s_LogLevel;

  /// \brief Counts the number of messages of each type.
  static ezAtomicInteger32 s_uiMessageCount[EventType::ENUM_COUNT];

  /// \brief Manages all the Event Handlers for the logging events.
  static ezEvent<const LoggingEvent&, void*, ezStaticAllocatorWrapper> s_LoggingEvent;

  static void BroadcastLoggingEvent(EventType::Enum type, const char* szString);

  static void WriteBlockHeader(ezLogBlock* pBlock);
};

/// \brief Instances of this class will group messages in a scoped block together.
class EZ_FOUNDATION_DLL ezLogBlock
{
public:
  ezLogBlock(const char* szName)
  {
    m_szName = szName;
    m_bWritten = false;

    if (ezThreadLocalStorage::IsInitialized())
    {
      m_pParentBlock = s_CurrentBlockTLS;
      s_CurrentBlockTLS = this;
    }
    else
    {
      m_pParentBlock = s_CurrentBlock;
      s_CurrentBlock = this;
    }
    
    m_iBlockDepth = m_pParentBlock ? (m_pParentBlock->m_iBlockDepth + 1) : 0;
  }

  ~ezLogBlock()
  {
    if (ezThreadLocalStorage::IsInitialized())
      s_CurrentBlockTLS = m_pParentBlock;
    else
      s_CurrentBlock = m_pParentBlock;

    ezLog::EndLogBlock(this);
  }

private:
  friend class ezLog;

  ezInt32 m_iBlockDepth;
  ezLogBlock* m_pParentBlock;
  const char* m_szName;
  bool m_bWritten;

  static ezLogBlock* s_CurrentBlock;
  static ezThreadLocalPointer<ezLogBlock> s_CurrentBlockTLS;
};

#include <Foundation/Logging/Implementation/Log_inl.h>

