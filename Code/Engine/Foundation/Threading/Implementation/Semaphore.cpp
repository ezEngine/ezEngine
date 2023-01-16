#include <Foundation/FoundationPCH.h>

#include <Foundation/Threading/Semaphore.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Threading/Implementation/Win/Semaphore_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Threading/Implementation/Posix/Semaphore_posix.h>
#else
#  error "Semaphore is not implemented on current platform"
#endif


EZ_STATICLINK_FILE(Foundation, Foundation_Threading_Implementation_Semaphore);
