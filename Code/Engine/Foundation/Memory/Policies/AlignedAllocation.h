#pragma once

#include <Foundation/Math/Math.h>

namespace ezMemoryPolicies
{
  /// \brief Allocation policy to support custom alignment per allocation.
  ///
  /// \see ezAllocator
  template <typename T>
  class ezAlignedAllocation
  {
  public:
    ezAlignedAllocation(ezAllocatorBase* pParent)
      : m_allocator(pParent)
    {
    }

    void* Allocate(size_t uiSize, size_t uiAlign)
    {
      EZ_ASSERT_DEV(uiAlign < (1 << 24), "Alignment of {0} is too big. Maximum supported alignment is 16MB.", uiAlign);

      const ezUInt32 uiPadding = (ezUInt32)(uiAlign - 1 + MetadataSize);
      const size_t uiAlignedSize = uiSize + uiPadding;

      ezUInt8* pMemory = (ezUInt8*)m_allocator.Allocate(uiAlignedSize, EZ_ALIGNMENT_MINIMUM);

      ezUInt8* pAlignedMemory = ezMemoryUtils::AlignBackwards(pMemory + uiPadding, uiAlign);

      ezUInt32* pMetadata = GetMetadataPtr(pAlignedMemory);
      *pMetadata = PackMetadata((ezUInt32)(pAlignedMemory - pMemory), (ezUInt32)uiAlign);

      return pAlignedMemory;
    }

    void Deallocate(void* pPtr)
    {
      const ezUInt32 uiOffset = UnpackOffset(GetMetadata(pPtr));
      ezUInt8* pMemory = static_cast<ezUInt8*>(pPtr) - uiOffset;
      m_allocator.Deallocate(pMemory);
    }

    size_t AllocatedSize(const void* pPtr)
    {
      const ezUInt32 uiMetadata = GetMetadata(pPtr);
      const ezUInt32 uiOffset = UnpackOffset(uiMetadata);
      const ezUInt32 uiAlign = UnpackAlignment(uiMetadata);
      const ezUInt32 uiPadding = uiAlign - 1 + MetadataSize;

      const ezUInt8* pMemory = static_cast<const ezUInt8*>(pPtr) - uiOffset;
      return m_allocator.AllocatedSize(pMemory) - uiPadding;
    }

    size_t UsedMemorySize(const void* pPtr)
    {
      const ezUInt32 uiOffset = UnpackOffset(GetMetadata(pPtr));
      const ezUInt8* pMemory = static_cast<const ezUInt8*>(pPtr) - uiOffset;
      return m_allocator.UsedMemorySize(pMemory);
    }

    EZ_ALWAYS_INLINE ezAllocatorBase* GetParent() const { return m_allocator.GetParent(); }

  private:
    enum
    {
      MetadataSize = sizeof(ezUInt32)
    };

    // Meta-data is stored 4 bytes before the aligned memory
    inline ezUInt32* GetMetadataPtr(void* pAlignedMemory)
    {
      return static_cast<ezUInt32*>(ezMemoryUtils::AddByteOffset(pAlignedMemory, -MetadataSize));
    }

    inline ezUInt32 GetMetadata(const void* pAlignedMemory)
    {
      return *static_cast<const ezUInt32*>(ezMemoryUtils::AddByteOffset(pAlignedMemory, -MetadataSize));
    }

    // Store offset between pMemory and pAlignedMemory in the lower 24 bit of meta-data.
    // The upper 8 bit are used to store the Log2 of the alignment.
    EZ_ALWAYS_INLINE ezUInt32 PackMetadata(ezUInt32 uiOffset, ezUInt32 uiAlignment) { return uiOffset | (ezMath::Log2i(uiAlignment) << 24); }

    EZ_ALWAYS_INLINE ezUInt32 UnpackOffset(ezUInt32 uiMetadata) { return uiMetadata & 0x00FFFFFF; }

    EZ_ALWAYS_INLINE ezUInt32 UnpackAlignment(ezUInt32 uiMetadata) { return 1 << (uiMetadata >> 24); }

    T m_allocator;
  };
} // namespace ezMemoryPolicies
