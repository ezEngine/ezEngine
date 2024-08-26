#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Application/Implementation/Win/ApplicationEntryPoint_win.h>
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>

namespace ezApplicationDetails
{
  void SetConsoleCtrlHandler(ezMinWindows::BOOL(EZ_WINDOWS_WINAPI* consoleHandler)(ezMinWindows::DWORD dwCtrlType))
  {
    ::SetConsoleCtrlHandler(consoleHandler, TRUE);
  }

  static ezMutex s_shutdownMutex;

  ezMutex& GetShutdownMutex()
  {
    return s_shutdownMutex;
  }

} // namespace ezApplicationDetails
#endif
