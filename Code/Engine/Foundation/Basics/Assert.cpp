#include <Foundation/PCH.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Strings/StringUtils.h>

bool ezDefaultAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  printf("%s(%u): Expression '%s' failed: %s\n", szSourceFile, uiLine, szExpression, szAssertMsg);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)

  // make sure the cursor is definitely shown, since the user must be able to click buttons
  ezInt32 iHideCursor = 1;
  while (ShowCursor(true) < 0)
    ++iHideCursor;

  #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

    ezInt32 iRes = _CrtDbgReport(_CRT_ASSERT, szSourceFile, uiLine, nullptr, "'%s'\nFunction: %s\nMessage: %s", szExpression, szFunction, szAssertMsg);

    // currently we will ALWAYS trigger the breakpoint / crash (except for when the user presses 'ignore')
    if (iRes == 0)
    {
      // when the user ignores the assert, restore the cursor show/hide state to the previous count
      for (ezInt32 i = 0; i < iHideCursor; ++i)
        ShowCursor(false);

      return false;
    }

  #else
    char szTemp[1024 * 4] = "";
    ezStringUtils::snprintf(szTemp, EZ_ARRAY_SIZE(szTemp), " *** Assertion ***\n\nExpression: \"%s\"\nFunction: \"%s\"\nFile: \"%s\"\nLine: %u\nMessage: \"%s\"", szExpression, szFunction, szSourceFile, uiLine, szAssertMsg);
    szTemp[1024 * 4 - 1] = '\0';
    MessageBox(nullptr, szTemp, "Assertion", MB_ICONERROR);
  #endif

#endif

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}

static ezAssertHandler g_AssertHandler = &ezDefaultAssertHandler;

ezAssertHandler ezGetAssertHandler()
{
  return g_AssertHandler;
}

void ezSetAssertHandler(ezAssertHandler handler)
{
  g_AssertHandler = handler;
}

bool ezFailedCheck(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szErrorMsg, ...)
{
  // always do a debug-break if no assert handler is installed
  if (g_AssertHandler == nullptr)
    return true;

  va_list ap;
  va_start (ap, szErrorMsg);

  char szMsg[1024 * 2] = "";
  ezStringUtils::vsnprintf(szMsg, EZ_ARRAY_SIZE(szMsg), szErrorMsg, ap);

  va_end (ap);

  return (*g_AssertHandler)(szSourceFile, uiLine, szFunction, szExpression, szMsg);
}




EZ_STATICLINK_FILE(Foundation, Foundation_Basics_Assert);

