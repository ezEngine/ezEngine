
#include <Foundation/PCH.h>
#include <Foundation/Threading/Thread.h>

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/OSThread_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/OSThread_posix.h>
#else
  #error "Thread functions are not implemented on current platform"
#endif





EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_OSThread);

