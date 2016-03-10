#include <Foundation/PCH.h>
#include <Foundation/Memory/Policies/GuardedAllocation.h>

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  #include <Foundation/Memory/Policies/Win/GuardedAllocation_win.h>
#else
  #error "ezGuardedAllocation is not implemented on current platform"
#endif
