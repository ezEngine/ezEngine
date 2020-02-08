#include <FoundationPCH.h>

#include <Foundation/Logging/ETWWriter.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Logging/Implementation/Win/ETWProvider_win.h>

void ezLogWriter::ETW::LogMessageHandler(const ezLoggingEventData& eventData)
{
  if (eventData.m_EventType == ezLogMsgType::Flush)
    return;

  ezETWProvider::GetInstance().LogMessge(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_szText);
}

void ezLogWriter::ETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, const char* szText)
{
  if (eventType == ezLogMsgType::Flush)
    return;

  ezETWProvider::GetInstance().LogMessge(eventType, uiIndentation, szText);
}

#else

void ezLogWriter::ETW::LogMessageHandler(const ezLoggingEventData& eventData)
{
}

void ezLogWriter::ETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, const char* szText)
{
}

#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_ETWWriter);
