#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/VisualStudioWriter.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>

void ezLogWriter::VisualStudio::LogMessageHandler(const ezLoggingEventData& eventData)
{
  if (eventData.m_EventType == ezLogMsgType::Flush)
    return;

#  if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  if (eventData.m_sTag.IsEqual_NoCase("beep"))
  {
    MessageBeep(0xFFFFFFFF);
  }
#  endif

  static ezMutex WriterLock; // will only be created if this writer is used at all
  EZ_LOCK(WriterLock);

  if (eventData.m_EventType == ezLogMsgType::BeginGroup)
    OutputDebugStringA("\n");

  for (ezUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    OutputDebugStringA(" ");

  ezStringBuilder s;

  switch (eventData.m_EventType)
  {
    case ezLogMsgType::BeginGroup:
      s.SetFormat("+++++ {} ({}) +++++\n", eventData.m_sText, eventData.m_sTag);
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::EndGroup:
#  if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      s.SetFormat("----- {} ({} sec) -----\n\n", eventData.m_sText, eventData.m_fSeconds);
#  else
      s.SetFormat("----- {} (timing info not available) -----\n\n", eventData.m_sText);
#  endif
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::ErrorMsg:
      s.SetFormat("Error: {}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::SeriousWarningMsg:
      s.SetFormat("Seriously: {}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::WarningMsg:
      s.SetFormat("Warning: {}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::SuccessMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::InfoMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::DevMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));
      break;

    case ezLogMsgType::DebugMsg:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));
      break;

    default:
      s.SetFormat("{}\n", eventData.m_sText);
      OutputDebugStringW(ezStringWChar(s));

      ezLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }
}

#else

void ezLogWriter::VisualStudio::LogMessageHandler(const ezLoggingEventData& eventData)
{
}

#endif
