#pragma once

#include <Foundation/Basics.h>

namespace ezMemoryPolicies
{
  /// \brief Aligned Heap memory allocation policy.
  ///
  /// \see ezAllocator
  class ezAlignedHeapAllocation
  {
  public:
    EZ_FORCE_INLINE ezAlignedHeapAllocation(ezAllocatorBase* pParent) { }
    EZ_FORCE_INLINE ~ezAlignedHeapAllocation() { }
    
    void* Allocate(size_t uiSize, size_t uiAlign);
    void Deallocate(void* ptr);
    
    EZ_FORCE_INLINE ezAllocatorBase* GetParent() const { return nullptr; }
  };

  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    #include <Foundation/Memory/Policies/Win/AlignedHeapAllocation_win.h>
  #elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX)
    #include <Foundation/Memory/Policies/Posix/AlignedHeapAllocation_posix.h>
  #else
    #error "ezAlignedHeapAllocation is not implemented on current platform"
  #endif
}

