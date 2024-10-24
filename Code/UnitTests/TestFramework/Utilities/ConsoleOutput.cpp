#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <ConsoleOutput_Platform.inl>

void OutputToConsole(ezTestOutput::Enum type, const char* szMsg)
{
  static ezInt32 iIndentation = 0;
  static bool bAnyError = false;

  ezUInt8 uiColor = 0x07;

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
    case ezTestOutput::Success:
      uiColor = 0x0A;
      break;
    case ezTestOutput::Message:
      uiColor = 0x0E;
      break;
    case ezTestOutput::Warning:
      uiColor = 0x0C;
      break;
    case ezTestOutput::Error:
      uiColor = 0x0C;
      bAnyError = true;
      break;
    case ezTestOutput::Duration:
    case ezTestOutput::ImageDiffFile:
    case ezTestOutput::InvalidType:
    case ezTestOutput::AllOutputTypes:
      return;

    case ezTestOutput::FinalResult:
      if (bAnyError)
        uiColor = 0x0C;
      else
        uiColor = 0x0A;

      // reset it for the next test round
      bAnyError = false;
      break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      break;
  }

  OutputToConsole_Platform(uiColor, type, iIndentation, szMsg);

  if (type >= ezTestOutput::Error)
  {
    fflush(stdout);
  }
}
