#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/ApplicationEntryPoint_Platform.h>
#  include <Foundation/Platform/Win/Utils/IncludeWindows.h>

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
