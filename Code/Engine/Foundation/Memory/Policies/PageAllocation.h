#pragma once

#include <Foundation/Basics.h>

namespace ezMemoryPolicies
{
  /// \brief This helper class can reserve and allocate whole memory pages.
  class ezPageAllocation
  {
  public:
    ezPageAllocation();
    ~ezPageAllocation();    

    void* PageAllocate(size_t uiSize);
    void PageDeallocate(void* ptr);
  };

  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    #include <Foundation/Memory/Policies/Win/PageAllocation_win.h>
  #elif EZ_ENABLED(EZ_PLATFORM_OSX)  || EZ_ENABLED(EZ_PLATFORM_LINUX)
    #include <Foundation/Memory/Policies/Posix/PageAllocation_posix.h>
  #else
    #error "ezPageAllocation is not implemented on current platform"
  #endif
}

