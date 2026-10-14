
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

void ezSuppressSystemErrorDialogs()
{
  // no Windows Error Reporting dialog when the process crashes
  SetErrorMode(GetErrorMode() | SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

  // no "abort() has been called" message box and no error report when abort() is called
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT) && defined(_DEBUG)
  // _CrtDbgReport() is used by the debug CRT's own checks and by third-party code (Qt reports fatal errors this way).
  // Send its reports to the debugger output instead of a dialog. The debug CRT is a DLL, so this applies to all modules.
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);
#endif
}
