#include <Foundation/PCH.h>
#include <Foundation/Threading/ThreadSignal.h>


#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Threading/Implementation/Win/ThreadSignal_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
  #include <Foundation/Threading/Implementation/Posix/ThreadSignal_posix.h>
#else
  #error "Unsupported Platform."
#endif



EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_ThreadSignal);

