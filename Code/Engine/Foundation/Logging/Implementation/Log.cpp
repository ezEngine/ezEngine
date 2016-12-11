#include <Foundation/PCH.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>

ezThreadLocalPointer<ezGlobalLog> ezGlobalLog::s_pInstances;
ezLogMsgType::Enum ezGlobalLog::s_LogLevel = ezLogMsgType::All;
ezAtomicInteger32 ezGlobalLog::s_uiMessageCount[ezLogMsgType::ENUM_COUNT];
ezLoggingEvent ezGlobalLog::s_LoggingEvent;

ezThreadLocalPointer<ezLogInterface> ezLog::s_DefaultLogSystem;

ezGlobalLog* ezGlobalLog::GetOrCreateInstance()
{
  ezGlobalLog* pLog = s_pInstances;

  if (pLog == nullptr)
  {
    // use new, not EZ_DEFAULT_NEW, to prevent tracking
    s_pInstances = new ezGlobalLog;
    return s_pInstances;
  }

  return pLog;
}

void ezGlobalLog::HandleLogMessage(const ezLoggingEventData& le)
{
  const ezLogMsgType::Enum ThisType = le.m_EventType;

  if ((ThisType > ezLogMsgType::None) && (ThisType < ezLogMsgType::All))
    s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  s_LoggingEvent.Broadcast(le);
}

void ezGlobalLog::SetLogLevel(ezLogMsgType::Enum LogLevel)
{
  EZ_ASSERT_DEV(LogLevel >= ezLogMsgType::None, "Invalid Log Level");
  EZ_ASSERT_DEV(LogLevel <= ezLogMsgType::All, "Invalid Log Level");

  s_LogLevel = LogLevel;
}


ezLogBlock::ezLogBlock(const char* szName, const char* szContextInfo)
{
  m_pLogInterface = ezGlobalLog::GetOrCreateInstance();

  if (!m_pLogInterface)
    return;

  m_szName = szName;
  m_szContextInfo = szContextInfo;
  m_bWritten = false;

  m_pParentBlock = m_pLogInterface->m_pCurrentBlock;
  m_pLogInterface->m_pCurrentBlock = this;

  m_iBlockDepth = m_pParentBlock ? (m_pParentBlock->m_iBlockDepth + 1) : 0;
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

  m_iBlockDepth = m_pParentBlock ? (m_pParentBlock->m_iBlockDepth + 1) : 0;
}

ezLogBlock::~ezLogBlock()
{
  if (!m_pLogInterface)
    return;

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
    le.m_uiIndentation = pBlock->m_iBlockDepth;
    le.m_szTag = pBlock->m_szContextInfo;

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
  le.m_uiIndentation = pBlock->m_iBlockDepth;
  le.m_szTag = pBlock->m_szContextInfo;

  pInterface->HandleLogMessage(le);
}

void ezLog::BroadcastLoggingEvent(ezLogInterface* pInterface, ezLogMsgType::Enum type, const char* szString)
{
  ezLogBlock* pTopBlock = pInterface->m_pCurrentBlock;
  ezInt32 iIndentation = 0;

  if (pTopBlock)
  {
    iIndentation = pTopBlock->m_iBlockDepth + 1;

    WriteBlockHeader(pInterface, pTopBlock);
  }

  char szTag[32] = "";

  if (ezStringUtils::StartsWith (szString, "["))
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
  le.m_uiIndentation = iIndentation;
  le.m_szTag = szTag;

  pInterface->HandleLogMessage(le);
}

void ezLog::SetDefaultLogSystem(ezLogInterface* pInterface)
{
  EZ_ASSERT_DEV(pInterface != nullptr, "You cannot set a nullptr logging system. If you want to discard all log information, set a dummy system that does not do anything.");

  s_DefaultLogSystem = pInterface;
}

ezLogInterface* ezLog::GetDefaultLogSystem()
{
  if (s_DefaultLogSystem == nullptr)
    s_DefaultLogSystem = ezGlobalLog::GetOrCreateInstance();

  return s_DefaultLogSystem;
}

void ezLog::Error(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(GetDefaultLogSystem(), ezLogMsgType::ErrorMsg, string.GetText(tmp));
}

void ezLog::SeriousWarning(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(GetDefaultLogSystem(), ezLogMsgType::SeriousWarningMsg, string.GetText(tmp));
}

void ezLog::Warning(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(GetDefaultLogSystem(), ezLogMsgType::WarningMsg, string.GetText(tmp));
}

void ezLog::Success(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(GetDefaultLogSystem(), ezLogMsgType::SuccessMsg, string.GetText(tmp));
}

void ezLog::Info(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(GetDefaultLogSystem(), ezLogMsgType::InfoMsg, string.GetText(tmp));
}

void ezLog::Dev(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(GetDefaultLogSystem(), ezLogMsgType::DevMsg, string.GetText(tmp));
}

void ezLog::Debug(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(GetDefaultLogSystem(), ezLogMsgType::DebugMsg, string.GetText(tmp));
}

#define LOG_IMPL(ThisType, pInterface) \
  if (pInterface == nullptr) \
    return; \
  char szString[4096]; \
  va_list args; \
  va_start (args, szFormat); \
  ezStringUtils::vsnprintf(szString, 4096, szFormat, args); \
  va_end (args); \
  BroadcastLoggingEvent(pInterface, ThisType, szString);

void ezLog::ErrorPrintf(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::ErrorMsg, GetDefaultLogSystem());
}

void ezLog::ErrorPrintfI(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::ErrorMsg, pInterface);
}

void ezLog::WarningPrintf(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::WarningMsg, GetDefaultLogSystem());
}

void ezLog::SuccessPrintf(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::SuccessMsg, GetDefaultLogSystem());
}

void ezLog::InfoPrintf(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::InfoMsg, GetDefaultLogSystem());
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Log);

