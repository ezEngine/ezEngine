
#include <Foundation/PCH.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Configuration/Startup.h>

EZ_BEGIN_SUBSYSTEM_DECLARATION(Foundation, Time)

  // no dependencies

  ON_BASE_STARTUP
  {
    ezSystemTime::Initialize();
  }

  ON_BASE_SHUTDOWN
  {
    ezSystemTime::Shutdown();
  }

EZ_END_SUBSYSTEM_DECLARATION

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Time/Implementation/Win/Time_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  #include <Foundation/Time/Implementation/Posix/Time_posix.h>
#else
  #error "Time functions are not implemented on current platform"
#endif

