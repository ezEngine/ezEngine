#include <Foundation/Logging/ETW.h>

void OutputToConsole_Platform(ezUInt8 uiColor, ezTestOutput::Enum type, ezInt32 iIndentation, const char* szMsg)
{
  printf("%*s%s\n", iIndentation, "", szMsg);

  char sz[4096];
  ezStringUtils::snprintf(sz, 4096, "%*s%s\n", iIndentation, "", szMsg);
  OutputDebugStringW(ezStringWChar(sz).GetData());

  ezLogMsgType::Enum logType = ezLogMsgType::None;
  switch (type)
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
    ezETW::LogMessage(logType, iIndentation, szMsg);
  }
}
