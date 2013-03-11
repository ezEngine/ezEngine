
#include <Foundation/PCH.h>
#include <Foundation/Threading/Thread.h>

// Include inline file
#if EZ_PLATFORM_WINDOWS
  #include <Foundation/Threading/Implementation/Win/Thread_win.h>
#else
  #error "Runnable thread entry functions are not implemented on current platform"
#endif
