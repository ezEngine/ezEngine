#include <Foundation/FoundationPCH.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS_UWP)
// For UWP we're currently using a mix of WinRT functions and Posix.
#  include <Foundation/Platform/Posix/OSFile_Posix.h>
#endif
