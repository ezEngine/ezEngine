#include <FoundationPCH.h>

#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Time/Timestamp.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <android/log.h>
#  define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "ezEngine", __VA_ARGS__)
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

static void SetConsoleColor(WORD ui)
{
#  if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#  endif
}
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
static void SetConsoleColor(ezUInt8 ui) {}
#else
#  error "Unknown Platform."
static void SetConsoleColor(ezUInt8 ui) {}
#endif

ezLog::TimestampMode ezLogWriter::Console::s_TimestampMode = ezLog::TimestampMode::None;

void ezLogWriter::Console::LogMessageHandler(const ezLoggingEventData& eventData)
{
  ezStringBuilder sTimestamp;
  ezLog::GenerateFormattedTimestamp(s_TimestampMode, sTimestamp);

  static ezMutex WriterLock; // will only be created if this writer is used at all
  EZ_LOCK(WriterLock);

  if (eventData.m_EventType == ezLogMsgType::BeginGroup)
    printf("\n");

  for (ezUInt32 i = 0; i < eventData.m_uiIndentation; ++i)
    printf(" ");

  switch (eventData.m_EventType)
  {
    case ezLogMsgType::Flush:
      fflush(stdout);
      break;

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
      printf("%sError: %s\n", sTimestamp.GetData(), eventData.m_szText);
      fflush(stdout);
      break;

    case ezLogMsgType::SeriousWarningMsg:
      SetConsoleColor(0x0C);
      printf("%sSeriously: %s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case ezLogMsgType::WarningMsg:
      SetConsoleColor(0x0E);
      printf("%sWarning: %s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case ezLogMsgType::SuccessMsg:
      SetConsoleColor(0x0A);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      fflush(stdout);
      break;

    case ezLogMsgType::InfoMsg:
      SetConsoleColor(0x07);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case ezLogMsgType::DevMsg:
      SetConsoleColor(0x08);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    case ezLogMsgType::DebugMsg:
      SetConsoleColor(0x09);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);
      break;

    default:
      SetConsoleColor(0x0D);
      printf("%s%s\n", sTimestamp.GetData(), eventData.m_szText);

      ezLog::Warning("Unknown Message Type {0}", eventData.m_EventType);
      break;
  }

  SetConsoleColor(0x07);
}

void ezLogWriter::Console::SetTimestampMode(ezLog::TimestampMode mode)
{
  s_TimestampMode = mode;
}
#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  undef printf
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Logging_Implementation_ConsoleWriter);
