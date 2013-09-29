#include <Foundation/PCH.h>
#include <Foundation/Basics/Assert.h>

bool ezDefaultAssertHandler(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  printf("%s(%u): Expression '%s' failed: %s\n", szSourceFile, uiLine, szExpression, szAssertMsg);

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)

    ezInt32 iRes = _CrtDbgReport(_CRT_ASSERT, szSourceFile, uiLine, NULL, "'%s'\nFunction: %s\nMessage: %s", szExpression, szFunction, szAssertMsg);

    // currently we will ALWAYS trigger the breakpoint / crash (except for when the user presses 'cancel'
    if (iRes == 0)
      return false;

  #else
    char szTemp[1024 * 4] = "";
    sprintf(szTemp, " *** Assertion ***\n\nExpression: \"%s\"\nFunction: \"%s\"\nFile: \"%s\"\nLine: %u\nMessage: \"%s\"", szExpression, szFunction, szSourceFile, uiLine, szAssertMsg);
    szTemp[1024 * 4 - 1] = '\0';
    MessageBox(NULL, szTemp, "Assertion", MB_ICONERROR);
  #endif

#endif

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}

ezAssertHandler g_AssertHandler = &ezDefaultAssertHandler;

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
  if (g_AssertHandler == NULL)
    return true;

  va_list ap;
  va_start (ap, szErrorMsg);

  char szMsg[1024 * 2] = "";
  vsprintf(szMsg, szErrorMsg, ap);
  szMsg[1024 * 2 - 1] = '\0';

  va_end (ap);

  return (*g_AssertHandler)(szSourceFile, uiLine, szFunction, szExpression, szMsg);
}


