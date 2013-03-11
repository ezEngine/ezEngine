
#include <Foundation/PCH.h>
#include <Foundation/Threading/Thread.h>

// Include inline file
#if EZ_PLATFORM_WINDOWS
  #include <Foundation/Threading/Implementation/Win/OSThread_win.h>
#else
  #error "Thread functions are not implemented on current platform"
#endif



