#include <FoundationPCH.h>

// Include inline file
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/ProcessGroup_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/ProcessGroup_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX)
#  include <Foundation/System/Implementation/Posix/ProcessGroup_posix.h>
#else
#  error "ProcessGroup functions are not implemented on current platform"
#endif

