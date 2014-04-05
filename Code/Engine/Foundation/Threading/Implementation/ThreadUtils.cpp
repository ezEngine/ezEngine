
#include <Foundation/PCH.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Threading/ThreadLocalStorage.h>
#include <Foundation/Configuration/Startup.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ThreadUtils)

  // no dependencies

  ON_BASE_STARTUP
  {
    ezThreadLocalStorage::Initialize();
    ezThreadUtils::Initialize();
  }

EZ_END_SUBSYSTEM_DECLARATION


// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/ThreadUtils_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/ThreadUtils_posix.h>
#else
  #error "ThreadUtils functions are not implemented on current platform"
#endif





EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadUtils);

