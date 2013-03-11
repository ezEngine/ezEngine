#include <Foundation/PCH.h>
#include <Foundation/Basics/Assert.h>

bool ezFailedCheck(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szErrorMsg, ...)
{
  va_list ap;
  va_start (ap, szErrorMsg);

  char szMsg[1024 * 2] = "";
  vsprintf(szMsg, szErrorMsg, ap);
  szMsg[1024 * 2 - 1] = '\0';

  va_end (ap);

  printf("%s(%u) : error 13: Expression '%s' failed: %s\n", szSourceFile, uiLine, szExpression, szMsg);

#if EZ_PLATFORM_WINDOWS
  #if EZ_COMPILE_FOR_DEBUG

    ezInt32 iRes = _CrtDbgReport(_CRT_ASSERT, szSourceFile, uiLine, NULL, "'%s'\nFunction: %s\nMessage: %s", szExpression, szFunction, szMsg);

    // currently we will ALWAYS trigger the breakpoint / crash (except for when the user presses 'cancel'
    if (iRes == 0)
      return false;

  #else
    char szTemp[1024 * 4] = "";
    sprintf(szTemp, " *** Assertion ***\n\nExpression: \"%s\"\nFunction: \"%s\"\nFile: \"%s\"\nLine: %u\nMessage: \"%s\"", szExpression, szFunction, szSourceFile, uiLine, szMsg);
    szTemp[1024 * 4 - 1] = '\0';
    MessageBox(NULL, szTemp, "Assertion", MB_ICONERROR);
  #endif

#else
  printf("\n *** Assertion *** \n");
  printf("Expression: \"%s\"\n", szExpression);
  printf("Function: \"%s\"\n", szFunction);
  printf("File: \"%s\"\n", szSourceFile);
  printf("Line: %u\n", uiLine);
  printf("Message: \"%s\"\n", szMsg);
  printf(" +++ Assertion +++ \n\n");
#endif

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}


