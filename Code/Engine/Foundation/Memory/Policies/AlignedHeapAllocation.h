#pragma once

#include <Foundation/Basics.h>

namespace ezMemoryPolicies
{
  /// \brief Aligned Heap memory allocation policy. Only supports one alignment for all allocations.
  ///
  /// \see ezAllocator
  template <size_t uiAlignment>
  class ezAlignedHeapAllocation
  {
  public:
    ezAlignedHeapAllocation(ezIAllocator* pParent);
    ~ezAlignedHeapAllocation();
    
    void* Allocate(size_t uiSize, size_t uiAlign);
    void Deallocate(void* ptr);
    
    size_t AllocatedSize(const void* ptr);
    size_t UsedMemorySize(const void* ptr);
    
    EZ_FORCE_INLINE ezIAllocator* GetParent() const { return NULL; }
  };

  #if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    #include <Foundation/Memory/Policies/Win/AlignedHeapAllocation_win.h>
  #elif EZ_ENABLED(EZ_PLATFORM_OSX)
    #include <Foundation/Memory/Policies/Posix/AlignedHeapAllocation_posix.h>
  #else
    #error "ezAlignedHeapAllocation is not implemented on current platform"
  #endif
}
