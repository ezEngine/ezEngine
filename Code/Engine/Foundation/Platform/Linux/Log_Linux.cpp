#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_LINUX)

#  include <Foundation/Logging/ETWWriter.h>
#  include <Foundation/Logging/Log.h>

void ezLog::Print(const char* szText)
{
  printf("%s", szText);

  ezLogWriter::ETW::LogMessage(ezLogMsgType::ErrorMsg, 0, szText);

  if (s_CustomPrintFunction)
  {
    s_CustomPrintFunction(szText);
  }

  fflush(stdout);
  fflush(stderr);
}

void ezLog::OsMessageBox(const ezFormatString& text)
{
  ezStringBuilder tmp;
  ezStringBuilder display = text.GetText(tmp);
  display.Trim(" \n\r\t");

  ezLog::Print(display);
  EZ_ASSERT_NOT_IMPLEMENTED;
}

#endif
