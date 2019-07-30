#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Application/Implementation/Win/ApplicationEntryPoint_win.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

namespace ezApplicationDetails
{
  void AttachToConsoleWindow(ezMinWindows::BOOL(EZ_WINDOWS_WINAPI* consoleHandler)(ezMinWindows::DWORD dwCtrlType))
  {
    // We're attaching to a potential console parent process
    // and ignore failure when no such parent is present
    if (AttachConsole(ATTACH_PARENT_PROCESS) == TRUE)
    {
      SetConsoleCtrlHandler(consoleHandler, TRUE);

      // this is necessary to direct the standard
      // output to the newly attached console
      freopen("CONOUT$", "w", stdout);
      freopen("CONOUT$", "w", stderr);
      freopen("CONIN$", "r", stdin);
      printf("\n");
    }
  }

  void DetachFromConsoleWindow()
  {
    if (HWND consoleWindow = GetConsoleWindow())
    {
      // Since the console already printed a new prompt the input pointer ends up in the blanks after
      // our output and no new prompt appears. Sending an enter keypress works around that limitation.
      PostMessageA(consoleWindow, WM_KEYDOWN, 0x0D, 0);
      PostMessageA(consoleWindow, WM_KEYUP, 0x0D, 0xC0000001);
    }
  }
} // namespace ezApplicationDetails
#endif
