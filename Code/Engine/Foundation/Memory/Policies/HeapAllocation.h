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
    EZ_ALWAYS_INLINE ezHeapAllocation(ezAllocatorBase* pParent) { }
    EZ_ALWAYS_INLINE ~ezHeapAllocation() { }

    EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      void* ptr = malloc(uiSize);

      // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
      // if these asserts fail, you need to check what container made the allocation and change it
      // to use an aligned allocator, e.g. ezAlignedAllocatorWrapper

      /// \todo Actually the assert should check for 4, not 8, but then stuff fails (tm) ... someone should investigate
      EZ_ASSERT_DEBUG(uiAlign <= 8, "This allocator does not guarantee alignments larger than 4. Use an aligned allocator to allocate the desired data type.");
      EZ_CHECK_ALIGNMENT(ptr, uiAlign);

      return ptr;
    }

    EZ_FORCE_INLINE void* Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
    {
      void* result = realloc(ptr, uiNewSize);
      EZ_CHECK_ALIGNMENT(result, uiAlign);

      return result;
    }

    EZ_ALWAYS_INLINE void Deallocate(void* ptr)
    {
      free(ptr);
    }
     
    EZ_ALWAYS_INLINE ezAllocatorBase* GetParent() const { return nullptr; }
  };
}

