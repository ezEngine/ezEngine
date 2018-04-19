#include <PCH.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Time/Time.h>

ezLogMsgType::Enum ezLog::s_LogLevel = ezLogMsgType::All;
ezAtomicInteger32 ezGlobalLog::s_uiMessageCount[ezLogMsgType::ENUM_COUNT];
ezLoggingEvent ezGlobalLog::s_LoggingEvent;

/// \brief The log system that messages are sent to when the user specifies no system himself.
static thread_local ezLogInterface* s_DefaultLogSystem = nullptr;


void ezGlobalLog::AddLogWriter(ezLoggingEvent::Handler handler)
{
  if (!s_LoggingEvent.HasEventHandler(handler))
  {
    s_LoggingEvent.AddEventHandler(handler);
  }
}

void ezGlobalLog::RemoveLogWriter(ezLoggingEvent::Handler handler)
{
  s_LoggingEvent.RemoveEventHandler(handler);
}

void ezGlobalLog::HandleLogMessage(const ezLoggingEventData& le)
{
  const ezLogMsgType::Enum ThisType = le.m_EventType;

  if ((ThisType > ezLogMsgType::None) && (ThisType < ezLogMsgType::All))
    s_uiMessageCount[ThisType].Increment();

  s_LoggingEvent.Broadcast(le);
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

  m_iBlockDepth = m_pParentBlock ? (m_pParentBlock->m_iBlockDepth + 1) : 0;

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

  m_iBlockDepth = m_pParentBlock ? (m_pParentBlock->m_iBlockDepth + 1) : 0;

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
#else
  m_fSeconds = 0;
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
    le.m_uiIndentation = pBlock->m_iBlockDepth;
    le.m_szTag = pBlock->m_szContextInfo;
    le.m_fSeconds = pBlock->m_fSeconds;

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
  if (pInterface->GetLogLevel() < type)
    return;

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

void ezLog::Error(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::ErrorMsg, string.GetText(tmp));
}

void ezLog::SeriousWarning(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::SeriousWarningMsg, string.GetText(tmp));
}

void ezLog::Warning(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::WarningMsg, string.GetText(tmp));
}

void ezLog::Success(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::SuccessMsg, string.GetText(tmp));
}

void ezLog::Info(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::InfoMsg, string.GetText(tmp));
}

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)

void ezLog::Dev(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::DevMsg, string.GetText(tmp));
}

#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

void ezLog::Debug(ezLogInterface* pInterface, const ezFormatString& string)
{
  if (pInterface == nullptr)
    return;

  ezStringBuilder tmp;
  BroadcastLoggingEvent(pInterface, ezLogMsgType::DebugMsg, string.GetText(tmp));
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_Log);

