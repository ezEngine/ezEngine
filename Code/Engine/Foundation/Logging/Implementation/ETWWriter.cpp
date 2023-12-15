#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ETWWriter.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#  include <Foundation/Platform/Win/ETWProvider_Win.h>

void ezLogWriter::ETW::LogMessageHandler(const ezLoggingEventData& eventData)
{
  if (eventData.m_EventType == ezLogMsgType::Flush)
    return;

  ezETWProvider::GetInstance().LogMessge(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_sText);
}

void ezLogWriter::ETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText)
{
  if (eventType == ezLogMsgType::Flush)
    return;

  ezETWProvider::GetInstance().LogMessge(eventType, uiIndentation, sText);
}

#else

void ezLogWriter::ETW::LogMessageHandler(const ezLoggingEventData& eventData) {}

void ezLogWriter::ETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText) {}

#endif


