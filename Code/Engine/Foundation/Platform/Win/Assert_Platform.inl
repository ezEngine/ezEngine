
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS) && EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
#  include <crtdbg.h>
#endif

#if EZ_ENABLED(EZ_COMPILER_MSVC)
void MSVC_OutOfLine_DebugBreak(...)
{
  __debugbreak();
}
#endif

bool ezDefaultAssertHandler_Platform(const char* szSourceFile, ezUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg, const char* szTemp)
{
  EZ_IGNORE_UNUSED(szSourceFile);
  EZ_IGNORE_UNUSED(uiLine);
  EZ_IGNORE_UNUSED(szFunction);
  EZ_IGNORE_UNUSED(szExpression);
  EZ_IGNORE_UNUSED(szAssertMsg);
  EZ_IGNORE_UNUSED(szTemp);

  // make sure the cursor is definitely shown, since the user must be able to click buttons
  ezInt32 iHideCursor = 1;
  while (ShowCursor(true) < 0)
    ++iHideCursor;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG) && defined(_DEBUG)

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

  MessageBoxA(nullptr, szTemp, "Assertion", MB_ICONERROR);

#endif

  // always do a debug-break
  // in release-builds this will just crash the app
  return true;
}
