#include <FoundationPCH.h>

#include <Foundation/Logging/ConsoleWriter.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

static void SetConsoleColor(WORD ui)
{
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#endif
}
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
static void SetConsoleColor(ezUInt8 ui) {}
#else
#error "Unknown Platform."
static void SetConsoleColor(ezUInt8 ui) {}
#endif

void ezLogWriter::Console::LogMessageHandler(const ezLoggingEventData& eventData)
{
  static ezMutex WriterLock; // will only be created if this writer is used at all
  EZ_LOCK(WriterLock);

  if (eventData.m_EventType == ezLogMsgType::BeginGroup)
    printf("\n");

  for (ezUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    printf(" ");

  switch (eventData.m_EventType)
  {
    case ezLogMsgType::BeginGroup:
      SetConsoleColor(0x02);
      printf("+++++ %s (%s) +++++\n", eventData.m_szText, eventData.m_szTag);
      break;
    case ezLogMsgType::EndGroup:
      SetConsoleColor(0x02);
#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
      printf("----- %s (%.6f sec)-----\n\n", eventData.m_szText, eventData.m_fSeconds);
#else
      printf("----- %s (%s)-----\n\n", eventData.m_szText, "timing info not available");
#endif
      break;
    case ezLogMsgType::ErrorMsg:
      SetConsoleColor(0x0C);
      printf("Error: %s\n", eventData.m_szText);
      break;
    case ezLogMsgType::SeriousWarningMsg:
      SetConsoleColor(0x0C);
      printf("Seriously: %s\n", eventData.m_szText);
      break;
    case ezLogMsgType::WarningMsg:
      SetConsoleColor(0x0E);
      printf("Warning: %s\n", eventData.m_szText);
      break;
    case ezLogMsgType::SuccessMsg:
      SetConsoleColor(0x0A);
      printf("%s\n", eventData.m_szText);
      break;
    case ezLogMsgType::InfoMsg:
      SetConsoleColor(0x07);
      printf("%s\n", eventData.m_szText);
      break;
    case ezLogMsgType::DevMsg:
      SetConsoleColor(0x08);
      printf("%s\n", eventData.m_szText);
      break;
    case ezLogMsgType::DebugMsg:
      SetConsoleColor(0x09);
      printf("%s\n", eventData.m_szText);
      break;
    default:
      SetConsoleColor(0x0D);
      printf("%s\n", eventData.m_szText);

      ezLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }

  SetConsoleColor(0x07);
}



EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_ConsoleWriter);

