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
    EZ_ALWAYS_INLINE ezHeapAllocation(ezAllocatorBase* pParent) {}
    EZ_ALWAYS_INLINE ~ezHeapAllocation() {}

    EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      // malloc has no alignment guarantees, even though on many systems it returns 16 byte aligned data
      // if these asserts fail, you need to check what container made the allocation and change it
      // to use an aligned allocator, e.g. ezAlignedAllocatorWrapper

      // unfortunately using EZ_ALIGNMENT_MINIMUM doesn't work, because even on 32 Bit systems we try to do allocations with 8 Byte
      // alignment interestingly, the code that does that, seems to work fine anyway
      EZ_ASSERT_DEBUG(
          uiAlign <= 8,
          "This allocator does not guarantee alignments larger than 8. Use an aligned allocator to allocate the desired data type.");

      void* ptr = malloc(PadSize(uiSize));
      EZ_CHECK_ALIGNMENT(ptr, uiAlign);

      return OffsetPtr(ptr);
    }

    EZ_FORCE_INLINE void* Reallocate(void* currentPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
    {
      void* ptr = realloc(RestorePtr(currentPtr), PadSize(uiNewSize));
      EZ_CHECK_ALIGNMENT(ptr, uiAlign);

      return OffsetPtr(ptr);
    }

    EZ_ALWAYS_INLINE void Deallocate(void* ptr) { free(RestorePtr(ptr)); }

    EZ_ALWAYS_INLINE ezAllocatorBase* GetParent() const { return nullptr; }

  private:
    EZ_ALWAYS_INLINE size_t PadSize(size_t uiSize)
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      return uiSize + 2 * EZ_ALIGNMENT_MINIMUM;
#else
      return uiSize;
#endif
    }

    EZ_ALWAYS_INLINE void* OffsetPtr(void* ptr)
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      ezUInt32 uiOffset = ezMemoryUtils::IsAligned(ptr, 2 * EZ_ALIGNMENT_MINIMUM) ? EZ_ALIGNMENT_MINIMUM : 2 * EZ_ALIGNMENT_MINIMUM;
      ptr = ezMemoryUtils::AddByteOffset(ptr, uiOffset - 4);
      *static_cast<ezUInt32*>(ptr) = uiOffset;
      return ezMemoryUtils::AddByteOffset(ptr, 4);
#else
      return ptr;
#endif
    }

    EZ_ALWAYS_INLINE void* RestorePtr(void* ptr)
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      ptr = ezMemoryUtils::AddByteOffset(ptr, -4);
      ezInt32 uiOffset = *static_cast<ezUInt32*>(ptr);
      return ezMemoryUtils::AddByteOffset(ptr, -uiOffset + 4);
#else
      return ptr;
#endif
    }
  };
}

