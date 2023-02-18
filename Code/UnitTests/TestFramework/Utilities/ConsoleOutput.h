#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <android/log.h>
#endif
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Logging/ETWWriter.h>
inline void SetConsoleColorInl(WORD ui)
{
#  if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#  endif
}
#else
inline void SetConsoleColorInl(ezUInt8 ui) {}
#endif

inline void OutputToConsole(ezTestOutput::Enum type, const char* szMsg)
{
  static ezInt32 iIndentation = 0;
  static bool bAnyError = false;

  switch (type)
  {
    case ezTestOutput::StartOutput:
      break;
    case ezTestOutput::BeginBlock:
      iIndentation += 2;
      break;
    case ezTestOutput::EndBlock:
      iIndentation -= 2;
      break;
    case ezTestOutput::Details:
      SetConsoleColorInl(0x07);
      break;
    case ezTestOutput::ImportantInfo:
      SetConsoleColorInl(0x07);
      break;
    case ezTestOutput::Success:
      SetConsoleColorInl(0x0A);
      break;
    case ezTestOutput::Message:
      SetConsoleColorInl(0x0E);
      break;
    case ezTestOutput::Warning:
      SetConsoleColorInl(0x0C);
      break;
    case ezTestOutput::Error:
      SetConsoleColorInl(0x0C);
      bAnyError = true;
      break;
    case ezTestOutput::Duration:
    case ezTestOutput::ImageDiffFile:
    case ezTestOutput::InvalidType:
    case ezTestOutput::AllOutputTypes:
      return;

    case ezTestOutput::FinalResult:
      if (bAnyError)
        SetConsoleColorInl(0x0C);
      else
        SetConsoleColorInl(0x0A);

      // reset it for the next test round
      bAnyError = false;
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  printf("%*s%s\n", iIndentation, "", szMsg);
  SetConsoleColorInl(0x07);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  ezLogMsgType::Enum logType = ezLogMsgType::None;
  switch (Type)
  {
    case ezTestOutput::StartOutput:
    case ezTestOutput::InvalidType:
    case ezTestOutput::AllOutputTypes:
      logType = ezLogMsgType::None;
      break;
    case ezTestOutput::BeginBlock:
      logType = ezLogMsgType::BeginGroup;
      break;
    case ezTestOutput::EndBlock:
      logType = ezLogMsgType::EndGroup;
      break;
    case ezTestOutput::ImportantInfo:
    case ezTestOutput::Details:
    case ezTestOutput::Message:
    case ezTestOutput::Duration:
    case ezTestOutput::FinalResult:
      logType = ezLogMsgType::InfoMsg;
      break;
    case ezTestOutput::Success:
      logType = ezLogMsgType::SuccessMsg;
      break;
    case ezTestOutput::Warning:
      logType = ezLogMsgType::WarningMsg;
      break;
    case ezTestOutput::Error:
      logType = ezLogMsgType::ErrorMsg;
      break;
    case ezTestOutput::ImageDiffFile:
      logType = ezLogMsgType::DevMsg;
      break;
    default:
      break;
  }
  if (logType != ezLogMsgType::None)
  {
    ezLogWriter::ETW::LogMessage(ezLogMsgType::InfoMsg, iIndentation, szMsg);
  }
#endif
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  char sz[4096];
  ezStringUtils::snprintf(sz, 4096, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(ezStringWChar(sz).GetData());
#endif
#if EZ_ENABLED(EZ_PLATFORM_ANDROID)
  __android_log_print(ANDROID_LOG_DEBUG, "ezEngine", "%*s%s\n", iIndentation, "", szMsg);
#endif

  if (type >= ezTestOutput::Error)
  {
    fflush(stdout);
  }
}
