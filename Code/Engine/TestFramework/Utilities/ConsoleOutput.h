#pragma once

#include <TestFramework/Framework/TestFramework.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
inline void SetConsoleColorInl(WORD ui)
{
#if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#endif
}
#else
inline void SetConsoleColorInl(ezUInt8 ui) {}
#endif

inline void OutputToConsole(ezTestOutput::Enum Type, const char* szMsg)
{
  static ezInt32 iIndentation = 0;
  static bool bAnyError = false;

  switch (Type)
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
    case ezTestOutput::FinalResult:
      if (bAnyError)
        SetConsoleColorInl(0x0C);
      else
        SetConsoleColorInl(0x0A);

      // reset it for the next test round
      bAnyError = false;
      break;
    default:
      break;
  }

  printf("%*s%s\n", iIndentation, "", szMsg);
  SetConsoleColorInl(0x07);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  char sz[4096];
  ezStringUtils::snprintf(sz, 1024, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(ezStringWChar(sz).GetData());
#endif
  if (Type >= ezTestOutput::Error)
  {
    fflush(stdout);
  }
}
