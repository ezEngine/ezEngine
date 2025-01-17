#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Application.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Time/Timestamp.h>

#include <stdarg.h>

// Comment in to log into ezLog::Print any message that is output while no logger is registered.
// #define DEBUG_STARTUP_LOGGING

ezLogMsgType::Enum ezLog::s_DefaultLogLevel = ezLogMsgType::All;
ezLog::PrintFunction ezLog::s_CustomPrintFunction = nullptr;
ezAtomicInteger32 ezGlobalLog::s_uiMessageCount[ezLogMsgType::ENUM_COUNT];
ezLoggingEvent ezGlobalLog::s_LoggingEvent;
ezLogInterface* ezGlobalLog::s_pOverrideLog = nullptr;
static thread_local bool s_bAllowOverrideLog = true;
static ezMutex s_OverrideLogMutex;

/// \brief The log system that messages are sent to when the user specifies no system himself.
static thread_local ezLogInterface* s_DefaultLogSystem = nullptr;


ezEventSubscriptionID ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler handler)
{
  if (s_LoggingEvent.HasEventHandler(handler))
    return 0;

  return s_LoggingEvent.AddEventHandler(handler);
}

void ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  if (!s_LoggingEvent.HasEventHandler(handler))
    return;

  s_LoggingEvent.RemoveEventHandler(handler);
}

void ezGlobalLog::RemoveLogWriter(ezEventSubscriptionID& ref_subscriptionID)
{
  s_LoggingEvent.RemoveEventHandler(ref_subscriptionID);
}

void ezGlobalLog::SetGlobalLogOverride(ezLogInterface* pInterface)
{
  EZ_LOCK(s_OverrideLogMutex);

  EZ_ASSERT_DEV(pInterface == nullptr || s_pOverrideLog == nullptr, "Only one override log can be set at a time");
  s_pOverrideLog = pInterface;
}

void ezGlobalLog::HandleLogMessage(const ezLoggingEventData& le)
{
  if (s_pOverrideLog != nullptr && s_pOverrideLog != this && s_bAllowOverrideLog)
  {
    // only enter the lock when really necessary
    EZ_LOCK(s_OverrideLogMutex);

    // since s_bAllowOverrideLog is thread_local we do not need to re-check it

    // check this again under the lock, to be safe
    if (s_pOverrideLog != nullptr && s_pOverrideLog != this)
    {
      // disable the override log for the period in which it handles the event
      // to prevent infinite recursions
      s_bAllowOverrideLog = false;
      s_pOverrideLog->HandleLogMessage(le);
      s_bAllowOverrideLog = true;

      return;
    }
  }

  // else
  {
    const ezLogMsgType::Enum ThisType = le.m_EventType;

    if ((ThisType > ezLogMsgType::None) && (ThisType < ezLogMsgType::All))
      s_uiMessageCount[ThisType].Increment();

#ifdef DEBUG_STARTUP_LOGGING
    if (s_LoggingEvent.IsEmpty())
    {
      ezStringBuilder stmp = le.m_sText;
      stmp.Append("\n");
      ezLog::Print(stmp);
    }
#endif
    s_LoggingEvent.Broadcast(le);
  }
}

ezLogBlock::ezLogBlock(ezStringView sName, ezStringView sContextInfo)
{
  m_pLogInterface = ezLog::GetThreadLocalLogSystem();

  if (!m_pLogInterface)
    return;

  m_sName = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = ezTime::Now().GetSeconds();
#endif
}


ezLogBlock::ezLogBlock(ezLogInterface* pInterface, ezStringView sName, ezStringView sContextInfo)
{
  m_pLogInterface = pInterface;

  if (!m_pLogInterface)
    return;

  m_sName = sName;
  m_sContextInfo = sContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = ezTime::Now().GetSeconds();
#endif
}

ezLogBlock::~ezLogBlock()
{
  if (!m_pLogInterface)
    return;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = ezTime::Now().GetSeconds() - m_fSeconds;
#endif

  m_pLogInterface->m_pCurrentBlock = m_pParentBlock;

  ezLog::EndLogBlock(m_pLogInterface, this);
}


void ezLog::EndLogBlock(ezLogInterface* pInterface, ezLogBlock* pBlock)
{
  if (pBlock->m_bWritten)
  {
    ezLoggingEventData le;
    le.m_EventType = ezLogMsgType::EndGroup;
    le.m_sText = pBlock->m_sName;
    le.m_uiIndentation = pBlock->m_uiBlockDepth;
    le.m_sTag = pBlock->m_sContextInfo;
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    le.m_fSeconds = pBlock->m_fSeconds;
#endif

    pInterface->HandleLogMessage(le);
  }
}

void ezLog::WriteBlockHeader(ezLogInterface* pInterface, ezLogBlock* pBlock)
{
  if (!pBlock || pBlock->m_bWritten)
    return;

  pBlock->m_bWritten = true;

  WriteBlockHeader(pInterface, pBlock->m_pParentBlock);

  ezLoggingEventData le;
  le.m_EventType = ezLogMsgType::BeginGroup;
  le.m_sText = pBlock->m_sName;
  le.m_uiIndentation = pBlock->m_uiBlockDepth;
  le.m_sTag = pBlock->m_sContextInfo;

  pInterface->HandleLogMessage(le);
}

void ezLog::BroadcastLoggingEvent(ezLogInterface* pInterface, ezLogMsgType::Enum type, ezStringView sString)
{
  ezLogBlock* pTopBlock = pInterface->m_pCurrentBlock;
  ezUInt8 uiIndentation = 0;

  if (pTopBlock)
  {
    uiIndentation = pTopBlock->m_uiBlockDepth + 1;

    WriteBlockHeader(pInterface, pTopBlock);
  }

  char szTag[32] = "";

  if (sString.StartsWith("["))
  {
    const char* szAfterTag = sString.GetStartPointer();

    ++szAfterTag;

    ezInt32 iPos = 0;

    // only treat it as a tag, if it is properly enclosed in square brackets and doesn't contain spaces
    while ((*szAfterTag != '\0') && (*szAfterTag != '[') && (*szAfterTag != ']') && (*szAfterTag != ' ') && (iPos < 31))
    {
      szTag[iPos] = *szAfterTag;
      ++szAfterTag;
      ++iPos;
    }

    if (*szAfterTag == ']')
    {
      szTag[iPos] = '\0';
      sString.SetStartPosition(szAfterTag + 1);
    }
    else
    {
      szTag[0] = '\0';
    }
  }

  ezLoggingEventData le;
  le.m_EventType = type;
  le.m_sText = sString;
  le.m_uiIndentation = uiIndentation;
  le.m_sTag = szTag;

  pInterface->HandleLogMessage(le);
  pInterface->m_uiLoggedMsgsSinceFlush++;
}

void ezLog::Printf(const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char buffer[4096];
  ezStringUtils::vsnprintf(buffer, EZ_ARRAY_SIZE(buffer), szFormat, args);

  Print(buffer);

  va_end(args);
}

void ezLog::SetCustomPrintFunction(PrintFunction func)
{
  s_CustomPrintFunction = func;
}

void ezLog::GenerateFormattedTimestamp(TimestampMode mode, ezStringBuilder& ref_sTimestampOut)
{
  // if mode is 'None', early out to not even retrieve a timestamp
  if (mode == TimestampMode::None)
  {
    return;
  }

  const ezDateTime dateTime = ezDateTime::MakeFromTimestamp(ezTimestamp::CurrentTimestamp());

  switch (mode)
  {
    case TimestampMode::Numeric:
      ref_sTimestampOut.SetFormat("[{}] ", ezArgDateTime(dateTime, ezArgDateTime::ShowDate | ezArgDateTime::ShowMilliseconds | ezArgDateTime::ShowTimeZone));
      break;
    case TimestampMode::TimeOnly:
      ref_sTimestampOut.SetFormat("[{}] ", ezArgDateTime(dateTime, ezArgDateTime::ShowMilliseconds));
      break;
    case TimestampMode::Textual:
      ref_sTimestampOut.SetFormat(
        "[{}] ", ezArgDateTime(dateTime, ezArgDateTime::TextualDate | ezArgDateTime::ShowMilliseconds | ezArgDateTime::ShowTimeZone));
      break;
    default:
      EZ_ASSERT_DEV(false, "Unknown timestamp mode.");
      break;
  }
}

void ezLog::SetThreadLocalLogSystem(ezLogInterface* pInterface)
{
  EZ_ASSERT_DEV(pInterface != nullptr, "You cannot set a nullptr logging system. If you want to discard all log information, set a dummy system that does not do anything.");

  s_DefaultLogSystem = pInterface;
}

ezLogInterface* ezLog::GetThreadLocalLogSystem()
{
  if (s_DefaultLogSystem == nullptr)
  {
    // use new, not EZ_DEFAULT_NEW, to prevent tracking
    s_DefaultLogSystem = new ezGlobalLog;
  }

  return s_DefaultLogSystem;
}

void ezLog::SetDefaultLogLevel(ezLogMsgType::Enum logLevel)
{
  EZ_ASSERT_DEV(logLevel >= ezLogMsgType::None && logLevel <= ezLogMsgType::All, "Invalid default log level {}", (int)logLevel);

  s_DefaultLogLevel = logLevel;
}

ezLogMsgType::Enum ezLog::GetDefaultLogLevel()
{
  return s_DefaultLogLevel;
}

#define LOG_LEVEL_FILTER(MaxLevel)                                                                                                  \
  if (pInterface == nullptr)                                                                                                        \
    return;                                                                                                                         \
  if ((pInterface->GetLogLevel() == ezLogMsgType::GlobalDefault ? ezLog::s_DefaultLogLevel : pInterface->GetLogLevel()) < MaxLevel) \
    return;


void ezLog::Error(ezLogInterface* pInterface, const ezFormatString& string)
{
  LOG_LEVEL_FILTER(ezLogMsgType::ErrorMsg);

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::ErrorMsg, string.GetText(tmp));
}

void ezLog::SeriousWarning(ezLogInterface* pInterface, const ezFormatString& string)
{
  LOG_LEVEL_FILTER(ezLogMsgType::SeriousWarningMsg);

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::SeriousWarningMsg, string.GetText(tmp));
}

void ezLog::Warning(ezLogInterface* pInterface, const ezFormatString& string)
{
  LOG_LEVEL_FILTER(ezLogMsgType::WarningMsg);

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::WarningMsg, string.GetText(tmp));
}

void ezLog::Success(ezLogInterface* pInterface, const ezFormatString& string)
{
  LOG_LEVEL_FILTER(ezLogMsgType::SuccessMsg);

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::SuccessMsg, string.GetText(tmp));
}

void ezLog::Info(ezLogInterface* pInterface, const ezFormatString& string)
{
  LOG_LEVEL_FILTER(ezLogMsgType::InfoMsg);

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::InfoMsg, string.GetText(tmp));
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

void ezLog::Dev(ezLogInterface* pInterface, const ezFormatString& string)
{
  LOG_LEVEL_FILTER(ezLogMsgType::DevMsg);

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::DevMsg, string.GetText(tmp));
}

#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

void ezLog::Debug(ezLogInterface* pInterface, const ezFormatString& string)
{
  LOG_LEVEL_FILTER(ezLogMsgType::DebugMsg);

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::DebugMsg, string.GetText(tmp));
}

#endif

bool ezLog::Flush(ezUInt32 uiNumNewMsgThreshold, ezTime timeIntervalThreshold, ezLogInterface* pInterface /*= GetThreadLocalLogSystem()*/)
{
  if (pInterface == nullptr || pInterface->m_uiLoggedMsgsSinceFlush == 0) // if really nothing was logged, don't execute a flush
    return false;

  const ezTime tNow = ezTime::Now();

  if (pInterface->m_uiLoggedMsgsSinceFlush <= uiNumNewMsgThreshold && tNow - pInterface->m_LastFlushTime < timeIntervalThreshold)
    return false;

  BroadcastLoggingEvent(pInterface, ezLogMsgType::Flush, nullptr);

  pInterface->m_uiLoggedMsgsSinceFlush = 0;
  pInterface->m_LastFlushTime = tNow;

  return true;
}
