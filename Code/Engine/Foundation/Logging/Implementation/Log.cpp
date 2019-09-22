#include <FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Time.h>
#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <android/log.h>
#endif
ezLogMsgType::Enum ezLog::s_DefaultLogLevel = ezLogMsgType::All;
ezAtomicInteger32 ezGlobalLog::s_uiMessageCount[ezLogMsgType::ENUM_COUNT];
ezLoggingEvent ezGlobalLog::s_LoggingEvent;
ezLogInterface* ezGlobalLog::s_pOverrideLog = nullptr;
static thread_local bool s_bAllowOverrideLog = true;
static ezMutex s_OverrideLogMutex;

/// \brief The log system that messages are sent to when the user specifies no system himself.
static thread_local ezLogInterface* s_DefaultLogSystem = nullptr;


ezEventSubscriptionID ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler handler)
{
  return s_LoggingEvent.AddEventHandler(handler);
}

void ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  s_LoggingEvent.RemoveEventHandler(handler);
}

void ezGlobalLog::RemoveLogWriter(ezEventSubscriptionID subscriptionID)
{
  s_LoggingEvent.RemoveEventHandler(subscriptionID);
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

    s_LoggingEvent.Broadcast(le);
  }
}

ezLogBlock::ezLogBlock(const char* szName, const char* szContextInfo)
{
  m_pLogInterface = ezLog::GetThreadLocalLogSystem();

  if (!m_pLogInterface)
    return;

  m_szName = szName;
  m_szContextInfo = szContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_uiBlockDepth = m_pParentBlock ? (m_pParentBlock->m_uiBlockDepth + 1) : 0;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  m_fSeconds = ezTime::Now().GetSeconds();
#endif
}


ezLogBlock::ezLogBlock(ezLogInterface* pInterface, const char* szName, const char* szContextInfo)
{
  m_pLogInterface = pInterface;

  if (!m_pLogInterface)
    return;

  m_szName = szName;
  m_szContextInfo = szContextInfo;
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
    le.m_szText = pBlock->m_szName;
    le.m_uiIndentation = pBlock->m_uiBlockDepth;
    le.m_szTag = pBlock->m_szContextInfo;
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
  le.m_szText = pBlock->m_szName;
  le.m_uiIndentation = pBlock->m_uiBlockDepth;
  le.m_szTag = pBlock->m_szContextInfo;

  pInterface->HandleLogMessage(le);
}

void ezLog::BroadcastLoggingEvent(ezLogInterface* pInterface, ezLogMsgType::Enum type, const char* szString)
{
  ezLogBlock* pTopBlock = pInterface->m_pCurrentBlock;
  ezUInt8 uiIndentation = 0;

  if (pTopBlock)
  {
    uiIndentation = pTopBlock->m_uiBlockDepth + 1;

    WriteBlockHeader(pInterface, pTopBlock);
  }

  char szTag[32] = "";

  if (ezStringUtils::StartsWith(szString, "["))
  {
    ++szString;

    ezInt32 iPos = 0;

    while ((*szString != '\0') && (*szString != '[') && (*szString != ']') && (iPos < 31))
    {
      szTag[iPos] = *szString;
      ++szString;
      ++iPos;
    }

    szTag[iPos] = '\0';

    if (*szString == ']')
      ++szString;
  }

  ezLoggingEventData le;
  le.m_EventType = type;
  le.m_szText = szString;
  le.m_uiIndentation = uiIndentation;
  le.m_szTag = szTag;

  pInterface->HandleLogMessage(le);
}

void ezLog::Printf(const char* szFormat, ...)
{
  va_list args;
  va_start(args, szFormat);

  char buffer[1024];
  ezStringUtils::vsnprintf(buffer, EZ_ARRAY_SIZE(buffer), szFormat, args);

  printf("%s", buffer);

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  OutputDebugStringA(buffer);
#  endif
#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_ERROR, "ezEngine", "%s", buffer);
#endif
  va_end(args);

  fflush(stdout);
  fflush(stderr);
}

void ezLog::SetThreadLocalLogSystem(ezLogInterface* pInterface)
{
  EZ_ASSERT_DEV(pInterface != nullptr,
    "You cannot set a nullptr logging system. If you want to discard all log information, set a dummy system that does not do anything.");

  s_DefaultLogSystem = pInterface;
}

ezLogInterface* ezLog::GetThreadLocalLogSystem()
{
  if (s_DefaultLogSystem == nullptr)
  {
    // use new, not EZ_DEFAULT_NEW, to prevent tracking
    s_DefaultLogSystem = new ezGlobalLog;
    s_DefaultLogSystem->SetLogLevel(s_DefaultLogLevel);
  }

  return s_DefaultLogSystem;
}

void ezLog::SetDefaultLogLevel(ezLogMsgType::Enum LogLevel)
{
  s_DefaultLogLevel = LogLevel;
}

void ezLog::Error(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr || pInterface->GetLogLevel() < ezLogMsgType::ErrorMsg)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::ErrorMsg, string.GetText(tmp));
}

void ezLog::SeriousWarning(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr || pInterface->GetLogLevel() < ezLogMsgType::SeriousWarningMsg)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::SeriousWarningMsg, string.GetText(tmp));
}

void ezLog::Warning(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr || pInterface->GetLogLevel() < ezLogMsgType::WarningMsg)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::WarningMsg, string.GetText(tmp));
}

void ezLog::Success(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr || pInterface->GetLogLevel() < ezLogMsgType::SuccessMsg)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::SuccessMsg, string.GetText(tmp));
}

void ezLog::Info(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr || pInterface->GetLogLevel() < ezLogMsgType::InfoMsg)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::InfoMsg, string.GetText(tmp));
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

void ezLog::Dev(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr || pInterface->GetLogLevel() < ezLogMsgType::DevMsg)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::DevMsg, string.GetText(tmp));
}

#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

void ezLog::Debug(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr || pInterface->GetLogLevel() < ezLogMsgType::DebugMsg)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::DebugMsg, string.GetText(tmp));
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Log);
