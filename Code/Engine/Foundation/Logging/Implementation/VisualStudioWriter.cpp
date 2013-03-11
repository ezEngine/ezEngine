#include <Foundation/PCH.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

#ifdef EZ_PLATFORM_WINDOWS

void ezLog_VisualStudioWriter::LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough)
{
  static ezMutex WriterLock; // will only be created if this writer is used at all
  ezLock<ezMutex> lock(WriterLock);

  if (EventData.m_EventType == ezLog::EventType::BeginGroup)
    OutputDebugString ("\n");

  for (ezUInt32 i = 0; i < EventData.m_uiIndentation; ++i)
    OutputDebugString (" ");

  char sz[4096];

  switch (EventData.m_EventType)
  {
  case ezLog::EventType::FlushToDisk:
    break;
  case ezLog::EventType::BeginGroup:
    ezStringUtils::snprintf(sz, 1024, "+++++ %s +++++\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::EndGroup:
    ezStringUtils::snprintf(sz, 1024, "----- %s -----\n\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::FatalErrorMsg:
    ezStringUtils::snprintf(sz, 1024, "Fatal Error: %s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::ErrorMsg:
    ezStringUtils::snprintf(sz, 1024, "Error: %s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::SeriousWarningMsg:
    ezStringUtils::snprintf(sz, 1024, "Seriously: %s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::WarningMsg:
    ezStringUtils::snprintf(sz, 1024, "Warning: %s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::SuccessMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::InfoMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::DevMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  case ezLog::EventType::DebugMsg:
  case ezLog::EventType::DebugRegularMsg:
    ezStringUtils::snprintf(sz, 1024, "%s\n", EventData.m_szText);
    OutputDebugString (sz);
    break;
  default:
    ezStringUtils::snprintf(sz, 1024, "%s\n", EventData.m_szText);
    OutputDebugString (sz);

    ezLog::Warning ("Unknown Message Type %d", EventData.m_EventType);
    break;
  }
}

#else

void ezLog_VisualStudioWriter::LogMessageHandler(const ezLog::LoggingEvent& EventData, void* pPassThrough)
{
}

#endif


