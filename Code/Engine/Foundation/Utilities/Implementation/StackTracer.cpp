
#include <Foundation/PCH.h>
#include <Foundation/Utilities/StackTracer.h>

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Utilities/Implementation/Win/StackTracer_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
  #include <Foundation/Utilities/Implementation/Posix/StackTracer_posix.h>
#else
  #error "StackTracer is not implemented on current platform"
#endif
