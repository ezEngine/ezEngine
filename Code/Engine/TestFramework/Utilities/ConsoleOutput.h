#pragma once

#include <TestFramework/Framework/TestFramework.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
inline void SetConsoleColorInl(WORD ui)
{
#  if EZ_DISABLED(EZ_PLATFORM_WINDOWS_UWP)
  // SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), ui);
#  endif
}
#else
inline void SetConsoleColorInl(ezUInt8 ui) {}
#endif

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_RESET "\x1b[0m"

inline void OutputToConsole(ezTestOutput::Enum Type, const char* szMsg)
{
  static ezInt32 iIndentation = 0;
  static bool bAnyError = false;

  FILE* outStream = stdout;

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
      fprintf(outStream, ANSI_COLOR_CYAN);
      break;
    case ezTestOutput::ImportantInfo:
      SetConsoleColorInl(0x07);
      fprintf(outStream, ANSI_COLOR_YELLOW);
      break;
    case ezTestOutput::Success:
      SetConsoleColorInl(0x0A);
      fprintf(outStream, ANSI_COLOR_GREEN);
      break;
    case ezTestOutput::Message:
      SetConsoleColorInl(0x0E);
      fprintf(outStream, ANSI_COLOR_RESET);
      break;
    case ezTestOutput::Warning:
      SetConsoleColorInl(0x0C);
      fprintf(outStream, ANSI_COLOR_YELLOW);
      break;
    case ezTestOutput::Error:
      SetConsoleColorInl(0x0C);
      bAnyError = true;
      outStream = stderr;
      fprintf(outStream, ANSI_COLOR_RED);
      break;
    case ezTestOutput::FinalResult:
      if (bAnyError)
      {
        SetConsoleColorInl(0x0C);
        fprintf(outStream, ANSI_COLOR_RED);
      }
      else
      {

        fprintf(outStream, ANSI_COLOR_GREEN);
        SetConsoleColorInl(0x0A);
      }

      // reset it for the next test round
      bAnyError = false;
      break;
    default:
      break;
  }

  fprintf(outStream, "%*s%s\n", iIndentation, "", szMsg);
  SetConsoleColorInl(0x07);
  fprintf(outStream, ANSI_COLOR_RESET);

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
