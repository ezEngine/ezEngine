#pragma once

#include <Foundation/Basics.h>

/// \brief Default heap memory allocation policy.
///
/// \see ezAllocatorWithPolicy
class ezAllocPolicyHeap
{
public:
  EZ_ALWAYS_INLINE ezAllocPolicyHeap(ezAllocator* pParent) {}
  EZ_ALWAYS_INLINE ~ezAllocPolicyHeap() = default;

  EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
    // if these asserts fail, you need to check what container made the allocation and change it
    // to use an aligned allocator, e.g. ezAlignedAllocatorWrapper

    // unfortunately using EZ_ALIGNMENT_MINIMUM doesn't work, because even on 32 Bit systems we try to do allocations with 8 Byte
    // alignment interestingly, the code that does that, seems to work fine anyway
    EZ_ASSERT_DEBUG(uiAlign <= 8, "This allocator does not guarantee alignments larger than 8. Use an aligned allocator to allocate the desired data type.");

    void* ptr = malloc(uiSize);
    EZ_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  EZ_FORCE_INLINE void* Reallocate(void* pCurrentPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
  {
    void* ptr = realloc(pCurrentPtr, uiNewSize);
    EZ_CHECK_ALIGNMENT(ptr, uiAlign);

    return ptr;
  }

  EZ_ALWAYS_INLINE void Deallocate(void* pPtr)
  {
    free(pPtr);
  }

  EZ_ALWAYS_INLINE ezAllocator* GetParent() const { return nullptr; }
};
