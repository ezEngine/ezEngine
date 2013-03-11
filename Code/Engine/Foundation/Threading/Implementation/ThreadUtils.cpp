
#include <Foundation/PCH.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Configuration/Startup.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, ThreadUtils)

  // no dependencies

  ON_BASE_STARTUP
  {
    ezThreadUtils::Initialize();
  }

  ON_BASE_SHUTDOWN
  {
    ezThreadUtils::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION


// Include inline file
#if EZ_PLATFORM_WINDOWS
  #include <Foundation/Threading/Implementation/Win/ThreadUtils_win.h>
#else
  #error "ThreadUtils functions are not implemented on current platform"
#endif



