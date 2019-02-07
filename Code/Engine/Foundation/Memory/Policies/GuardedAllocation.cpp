#include <FoundationPCH.h>

#include <Foundation/Memory/Policies/GuardedAllocation.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#include <Foundation/Memory/Policies/Win/GuardedAllocation_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
#include <Foundation/Memory/Policies/Posix/GuardedAllocation_posix.h>
#else
#error "ezGuardedAllocation is not implemented on current platform"
#endif

EZ_STATICLINK_FILE(Foundation, Foundation_Memory_Policies_GuardedAllocation);

