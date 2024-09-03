#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/ETWWriter.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS) || (EZ_ENABLED(EZ_PLATFORM_LINUX) && defined(BUILDSYSTEM_ENABLE_TRACELOGGING_LTTNG_SUPPORT))

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#    include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#    include <Foundation/Platform/Win/ETWProvider_Win.h>
#  else
#    include <Foundation/Platform/Linux/ETWProvider_Linux.h>
#  endif

void ezLogWriter::ETW::LogMessageHandler(const ezLoggingEventData& eventData)
{
  if (eventData.m_EventType == ezLogMsgType::Flush)
    return;

  ezETWProvider::GetInstance().LogMessage(eventData.m_EventType, eventData.m_uiIndentation, eventData.m_sText);
}

void ezLogWriter::ETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText)
{
  if (eventType == ezLogMsgType::Flush)
    return;

  ezETWProvider::GetInstance().LogMessage(eventType, uiIndentation, sText);
}

#else

void ezLogWriter::ETW::LogMessageHandler(const ezLoggingEventData& eventData) {}

void ezLogWriter::ETW::LogMessage(ezLogMsgType::Enum eventType, ezUInt8 uiIndentation, ezStringView sText) {}

#endif


