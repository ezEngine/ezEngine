#include <FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/System/Implementation/Win/MiniDumpUtils_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX)
#  include <Foundation/System/Implementation/OSX/MiniDumpUtils_OSX.h>
#elif EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/System/Implementation/Posix/MiniDumpUtils_posix.h>
#else
#  error "Mini-dump functions are not implemented on current platform"
#endif


