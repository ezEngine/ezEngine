#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/TraceWriter.h>
#include <Foundation/Tracing/TraceProvider.h>

void ezLogWriter::Tracing::LogMessageHandler(const ezLoggingEventData& eventData)
{
  const ezStringBuilder sTemp = eventData.m_sText;

  switch (eventData.m_EventType)
  {
    case ezLogMsgType::BeginGroup:
      EZ_TRACE_SCOPE_BEGIN("LogBlock", ezTraceLevel::Info,
        EZ_TRACE_VALUE("Text", sTemp.GetData()),
        EZ_TRACE_VALUE("Type", (int)eventData.m_EventType),
        EZ_TRACE_VALUE("Indentation", eventData.m_uiIndentation));
      break;
    case ezLogMsgType::EndGroup:
      EZ_TRACE_SCOPE_END("LogBlock");
      break;
    default:
      EZ_TRACE_EVENT("LogMessage", ezTraceLevel::Info,
        EZ_TRACE_VALUE("Text", sTemp.GetData()),
        EZ_TRACE_VALUE("Type", (int)eventData.m_EventType),
        EZ_TRACE_VALUE("Indentation", eventData.m_uiIndentation));
      break;
  }
}
