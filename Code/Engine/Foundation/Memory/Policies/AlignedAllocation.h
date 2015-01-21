#pragma once

#include <Foundation/Basics.h>
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
    ezAlignedAllocation(ezAllocatorBase* pParent) : 
      m_allocator(pParent)
    {
    }

    void* Allocate(size_t uiSize, size_t uiAlign)
    {
      EZ_ASSERT_DEV(uiAlign < (1 << 24), "Alignment of %d is too big. Maximum supported alignment is 16MB.", uiAlign);

      const ezUInt32 uiPadding = (ezUInt32)(uiAlign-1 + MetadataSize);
      const size_t uiAlignedSize = uiSize + uiPadding;

      ezUInt8* pMemory = (ezUInt8*)m_allocator.Allocate(uiAlignedSize, EZ_ALIGNMENT_MINIMUM);

      ezUInt8* pAlignedMemory = ezMemoryUtils::Align(pMemory + uiPadding, uiAlign);

      ezUInt32* pMetadata = GetMetadataPtr(pAlignedMemory);
      *pMetadata = PackMetadata((ezUInt32)(pAlignedMemory - pMemory), (ezUInt32)uiAlign);

      return pAlignedMemory;
    }

    void Deallocate(void* ptr)
    {
      const ezUInt32 uiOffset = UnpackOffset(GetMetadata(ptr));
      ezUInt8* pMemory = static_cast<ezUInt8*>(ptr) - uiOffset;
      m_allocator.Deallocate(pMemory);
    }

    size_t AllocatedSize(const void* ptr)
    {
      const ezUInt32 uiMetadata = GetMetadata(ptr);
      const ezUInt32 uiOffset = UnpackOffset(uiMetadata);
      const ezUInt32 uiAlign = UnpackAlignment(uiMetadata);
      const ezUInt32 uiPadding = uiAlign-1 + MetadataSize;

      const ezUInt8* pMemory = static_cast<const ezUInt8*>(ptr) - uiOffset;
      return m_allocator.AllocatedSize(pMemory) - uiPadding;
    }

    size_t UsedMemorySize(const void* ptr)
    {
      const ezUInt32 uiOffset = UnpackOffset(GetMetadata(ptr));
      const ezUInt8* pMemory = static_cast<const ezUInt8*>(ptr) - uiOffset;
      return m_allocator.UsedMemorySize(pMemory);
    }

    EZ_FORCE_INLINE ezAllocatorBase* GetParent() const
    { 
      return m_allocator.GetParent(); 
    }

  private:
    enum { MetadataSize = sizeof(ezUInt32) };

    // Meta-data is stored 4 bytes before the aligned memory
    inline ezUInt32* GetMetadataPtr(void* pAlignedMemory)
    {
      return static_cast<ezUInt32*>(ezMemoryUtils::AddByteOffset(pAlignedMemory, -MetadataSize));
    }

    inline ezUInt32 GetMetadata(const void* pAlignedMemory)
    {
      return *static_cast<const ezUInt32*>(ezMemoryUtils::AddByteOffsetConst(pAlignedMemory, 
        -MetadataSize));
    }

    // Store offset between pMemory and pAlignedMemory in the lower 24 bit of meta-data. 
    // The upper 8 bit are used to store the Log2 of the alignment.
    EZ_FORCE_INLINE ezUInt32 PackMetadata(ezUInt32 uiOffset, ezUInt32 uiAlignment)
    {
      return uiOffset | (ezMath::Log2i(uiAlignment) << 24);
    }

    EZ_FORCE_INLINE ezUInt32 UnpackOffset(ezUInt32 uiMetadata)
    {
      return uiMetadata & 0x00FFFFFF;
    }

    EZ_FORCE_INLINE ezUInt32 UnpackAlignment(ezUInt32 uiMetadata)
    {
      return 1 << (uiMetadata >> 24);
    }

    T m_allocator;
  };
}

