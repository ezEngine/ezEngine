#pragma once

#include <Foundation/Basics.h>

namespace ezMemoryPolicies
{
  /// \brief Default heap memory allocation policy.
  ///
  /// \see ezAllocator
  class ezHeapAllocation
  {
  public:
    ezHeapAllocation(ezIAllocator* pParent);
    ~ezHeapAllocation();    

    void* Allocate(size_t uiSize, size_t uiAlign);
    void Deallocate(void* ptr);
    
    size_t AllocatedSize(const void* ptr);
    size_t UsedMemorySize(const void* ptr);
    
    EZ_FORCE_INLINE ezIAllocator* GetParent() const { return NULL; }
  };

  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    #include <Foundation/Memory/Policies/Win/HeapAllocation_win.h>
  #elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
    #include <Foundation/Memory/Policies/Posix/HeapAllocation_posix.h>
  #else
    #error "ezHeapAllocation is not implemented on current platform"
  #endif
}
