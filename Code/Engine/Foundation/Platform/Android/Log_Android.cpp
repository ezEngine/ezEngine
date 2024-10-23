#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_ANDROID)

#  include <Foundation/Logging/Log.h>

#  include <android/log.h>

void ezLog::Print(const char* szText)
{
  printf("%s", szText);

  __android_log_print(ANDROID_LOG_ERROR, "ezEngine", "%s", szText);

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
