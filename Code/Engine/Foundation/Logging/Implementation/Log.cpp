#include <Foundation/PCH.h>
#include <Foundation/Logging/Log.h>

ezLog::EventType::Enum ezLog::s_LogLevel = ezLog::EventType::All;
ezAtomicInteger32 ezLog::s_uiMessageCount[EventType::ENUM_COUNT];
ezEvent<const ezLog::LoggingEvent&, void*, ezStaticAllocatorWrapper> ezLog::s_LoggingEvent;
ezLogBlock* ezLogBlock::s_CurrentBlock = NULL;
ezThreadLocalPointer<ezLogBlock> ezLogBlock::s_CurrentBlockTLS;

void ezLog::EndLogBlock (ezLogBlock* pBlock)
{
  if (pBlock->m_bWritten)
  {
    LoggingEvent le;
    le.m_EventType = EventType::EndGroup;
    le.m_szText = pBlock->m_szName;
    le.m_uiIndentation = pBlock->m_iBlockDepth;
    le.m_szTag = "";

    s_LoggingEvent.Broadcast(le);
  }
}

void ezLog::SetLogLevel(EventType::Enum LogLevel)
{
  EZ_ASSERT(LogLevel >= EventType::None, "Invalid Log Level");
  EZ_ASSERT(LogLevel <= EventType::All, "Invalid Log Level");

  s_LogLevel = LogLevel;
}

void ezLog::FlushToDisk()
{
  LoggingEvent le;
  le.m_EventType = EventType::FlushToDisk;

  s_LoggingEvent.Broadcast(le);
}

void ezLog::WriteBlockHeader(ezLogBlock* pBlock)
{
  if (!pBlock || pBlock->m_bWritten)
    return;

  pBlock->m_bWritten = true;

  WriteBlockHeader(pBlock->m_pParentBlock);

  LoggingEvent le;
  le.m_EventType = EventType::BeginGroup;
  le.m_szText = pBlock->m_szName;
  le.m_uiIndentation = pBlock->m_iBlockDepth;
  le.m_szTag = "";

  s_LoggingEvent.Broadcast(le);
}

void ezLog::BroadcastLoggingEvent(EventType::Enum type, const char* szString)
{
  ezLogBlock* pTopBlock = NULL;
  
  if (ezThreadLocalStorage::IsInitialized())
    pTopBlock = ezLogBlock::s_CurrentBlockTLS;
  else
    pTopBlock = ezLogBlock::s_CurrentBlock;

  ezInt32 iIndentation = 0;

  if (pTopBlock)
  {
    iIndentation = pTopBlock->m_iBlockDepth + 1;

    WriteBlockHeader(pTopBlock);
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

  LoggingEvent le;
  le.m_EventType = type;
  le.m_szText = szString;
  le.m_uiIndentation = iIndentation;
  le.m_szTag = szTag;

  s_LoggingEvent.Broadcast(le);
}

void ezLog::FatalError(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::FatalErrorMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

void ezLog::Error(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::ErrorMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

void ezLog::SeriousWarning(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::SeriousWarningMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

void ezLog::Warning(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::WarningMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

void ezLog::Success(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::SuccessMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

void ezLog::Info(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::InfoMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

#ifdef EZ_COMPILE_FOR_DEVELOPMENT

void ezLog::Dev(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::DevMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

#endif

#ifdef EZ_COMPILE_FOR_DEBUG

void ezLog::Debug(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::DebugMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

void ezLog::DebugRegular(const char* szFormat, ...)
{
  const EventType::Enum ThisType = EventType::DebugRegularMsg;

  s_uiMessageCount[ThisType].Increment();

  if (s_LogLevel < ThisType)
    return;

  char szString[4096];

  va_list args;
  va_start (args, szFormat);

  ezStringUtils::vsnprintf(szString, 4096, szFormat, args);

  va_end (args);

  BroadcastLoggingEvent (ThisType, szString);
}

#endif