#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

#  include <Foundation/Application/Application.h>
#  include <Foundation/Logging/ETW.h>
#  include <Foundation/Logging/Log.h>

void ezLog::Print(const char* szText)
{
  printf("%s", szText);

  ezETW::LogMessage(ezLogMsgType::ErrorMsg, 0, szText);
  OutputDebugStringW(ezStringWChar(szText).GetData());

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

#  if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  const char* title = "";
  if (ezApplication::GetApplicationInstance())
  {
    title = ezApplication::GetApplicationInstance()->GetApplicationName();
  }

  MessageBoxW(nullptr, ezStringWChar(display).GetData(), ezStringWChar(title), MB_OK);
#  else
  ezLog::Print(display);
  EZ_ASSERT_NOT_IMPLEMENTED;
#  endif
}

#endif
