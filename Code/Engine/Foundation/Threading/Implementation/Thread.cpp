
#include <Foundation/PCH.h>
#include <Foundation/Threading/Thread.h>

// Include inline file
#if EZ_PLATFORM_WINDOWS
  #include <Foundation/Threading/Implementation/Win/Thread_win.h>
#elif EZ_PLATFORM_OSX
  #include <Foundation/Threading/Implementation/Posix/Thread_posix.h>
#else
  #error "Runnable thread entry functions are not implemented on current platform"
#endif
