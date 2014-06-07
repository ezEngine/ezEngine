#include <Foundation/PCH.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

void ezLogWriter::VisualStudio::LogMessageHandler(const ezLoggingEventData& eventData)
{
  static ezMutex WriterLock; // will only be created if this writer is used at all
  EZ_LOCK(WriterLock);

  if (eventData.m_EventType == ezLogMsgType::BeginGroup)
    OutputDebugString ("\n");

  for (ezUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    OutputDebugString (" ");

  char sz[4096];

  switch (eventData.m_EventType)
  {
  case ezLogMsgType::BeginGroup:
    ezStringUtils::snprintf(sz, 1024, "+++++ %s (%s) +++++\n", eventData.m_szText, eventData.m_szTag);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::EndGroup:
    ezStringUtils::snprintf(sz, 1024, "----- %s -----\n\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::ErrorMsg:
    ezStringUtils::snprintf(sz, 1024, "Error: %s\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::SeriousWarningMsg:
    ezStringUtils::snprintf(sz, 1024, "Seriously: %s\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::WarningMsg:
    ezStringUtils::snprintf(sz, 1024, "Warning: %s\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::SuccessMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::InfoMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::DevMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLogMsgType::DebugMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
    OutputDebugString (sz);
    break;
  default:
    ezStringUtils::snprintf(sz, 1024, "%s\n", eventData.m_szText);
    OutputDebugString (sz);

    ezLog::Warning ("Unknown Message Type %d", eventData.m_EventType);
    break;
  }
}

#else

void ezLogWriter::VisualStudio::LogMessageHandler(const ezLoggingEventData& eventData)
{
}

#endif




EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_VisualStudioWriter);

