#include <PCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Time/Time.h>

// clang-format off
EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ThreadUtils)

  // no dependencies

  ON_BASESYSTEMS_STARTUP
  {
    ezThreadUtils::Initialize();
  }

EZ_END_SUBSYSTEM_DECLARATION;
// clang-format on

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <Foundation/Threading/Implementation/Win/ThreadUtils_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
#include <Foundation/Threading/Implementation/Posix/ThreadUtils_posix.h>
#else
#error "ThreadUtils functions are not implemented on current platform"
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadUtils);

