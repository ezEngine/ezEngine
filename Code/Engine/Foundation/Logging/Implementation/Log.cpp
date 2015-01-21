#include <Foundation/PCH.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Threading/ThreadUtils.h>


ezGlobalLog* ezGlobalLog::s_pInstance = nullptr;
ezLogMsgType::Enum ezGlobalLog::s_LogLevel = ezLogMsgType::All;
ezAtomicInteger32 ezGlobalLog::s_uiMessageCount[ezLogMsgType::ENUM_COUNT];
ezLoggingEvent ezGlobalLog::s_LoggingEvent;

ezLogInterface* ezLog::s_DefaultLogSystem = nullptr;

ezGlobalLog* ezGlobalLog::GetInstance()
{
  if (!ezThreadUtils::IsMainThread())
    return nullptr; /// \todo This is not so great, we should maybe somehow log this stuff

  static ezGlobalLog s_Log;
  return &s_Log;
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
  m_pLogInterface = ezGlobalLog::GetInstance();

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
    s_DefaultLogSystem = ezGlobalLog::GetInstance();

  return s_DefaultLogSystem;
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

void ezLog::Error(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::ErrorMsg, GetDefaultLogSystem());
}

void ezLog::Error(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::ErrorMsg, pInterface);
}

void ezLog::SeriousWarning(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::SeriousWarningMsg, GetDefaultLogSystem());
}

void ezLog::SeriousWarning(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::SeriousWarningMsg, pInterface);
}

void ezLog::Warning(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::WarningMsg, GetDefaultLogSystem());
}

void ezLog::Warning(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::WarningMsg, pInterface);
}

void ezLog::Success(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::SuccessMsg, GetDefaultLogSystem());
}

void ezLog::Success(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::SuccessMsg, pInterface);
}

void ezLog::Info(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::InfoMsg, GetDefaultLogSystem());
}

void ezLog::Info(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::InfoMsg, pInterface);
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

void ezLog::Dev(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::DevMsg, GetDefaultLogSystem());
}

void ezLog::Dev(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::DevMsg, pInterface);
}

#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

void ezLog::Debug(const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::DebugMsg, GetDefaultLogSystem());
}

void ezLog::Debug(ezLogInterface* pInterface, const char* szFormat, ...)
{
  LOG_IMPL(ezLogMsgType::DebugMsg, pInterface);
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Log);

