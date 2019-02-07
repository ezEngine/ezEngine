#include <FoundationPCH.h>

#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Strings/StringUtils.h>

bool ezDefaultAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  char szTemp[1024 * 4] = "";
  ezStringUtils::snprintf(szTemp, EZ_ARRAY_SIZE(szTemp), "\n\n *** Assertion ***\n\n    Expression: \"%s\"\n    Function: \"%s\"\n    File: \"%s\"\n    Line: %u\n    Message: \"%s\"\n\n", szExpression, szFunction, szSourceFile, uiLine, szAssertMsg);
  szTemp[1024 * 4 - 1] = '\0';

  printf("%s", szTemp);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  OutputDebugStringW(ezStringWChar(szTemp).GetData());
#endif

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  if (IsDebuggerPresent())
    return true;

// make sure the cursor is definitely shown, since the user must be able to click buttons
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
    // Todo: Use modern Windows API to show cursor in current window.
    // http://stackoverflow.com/questions/37956628/change-mouse-pointer-in-uwp-app
#else
  ezInt32 iHideCursor = 1;
  while (ShowCursor(true) < 0)
    ++iHideCursor;
#endif

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

  ezInt32 iRes = _CrtDbgReport(_CRT_ASSERT, szSourceFile, uiLine, nullptr, "'%s'\nFunction: %s\nMessage: %s", szExpression, szFunction, szAssertMsg);

  // currently we will ALWAYS trigger the breakpoint / crash (except for when the user presses 'ignore')
  if (iRes == 0)
  {
// when the user ignores the assert, restore the cursor show/hide state to the previous count
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
  // Todo: Use modern Windows API to restore cursor.
#else
    for (ezInt32 i = 0; i < iHideCursor; ++i)
      ShowCursor(false);
#endif

    return false;
  }

#else


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
  MessageBoxA(nullptr, szTemp, "Assertion", MB_ICONERROR);
#endif

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

bool ezFailedCheck(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* msg)
{
  // always do a debug-break if no assert handler is installed
  if (g_AssertHandler == nullptr)
    return true;

  return (*g_AssertHandler)(szSourceFile, uiLine, szFunction, szExpression, msg);
}

bool ezFailedCheck(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const class ezFormatString& msg)
{
  ezStringBuilder tmp;
  return ezFailedCheck(szSourceFile, uiLine, szFunction, szExpression, msg.GetText(tmp));
}


EZ_STATICLINK_FILE(Foundation, Foundation_Basics_Assert);

