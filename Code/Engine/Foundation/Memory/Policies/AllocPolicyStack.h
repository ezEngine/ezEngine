#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>

/// \brief This policy implements a stack allocator. It is designed for scenarios where you have a lot of short-lived allocations that are freed in a LIFO order,
/// but it also supports freeing in arbitrary order (at the cost of some fragmentation).
///
/// For debugging purposes, the policy can also overwrite all freed memory with 0xCDCDCDCD to make it easier to find use-after-free situations.
///
/// \see ezAllocatorWithPolicy
template <bool OverwriteMemoryOnFree = false>
class ezAllocPolicyStack
{
public:
  enum
  {
    Alignment = 16
  };

  EZ_FORCE_INLINE ezAllocPolicyStack(ezAllocator* pParent)
    : m_pParent(pParent)
    , m_uiMaxAllocSizeLog2(20) // 1024 * 1024 = 2^20
  {
  }

  EZ_FORCE_INLINE ~ezAllocPolicyStack()
  {
    EZ_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && m_uiCurrentBucketOffset == 0 && m_Allocations.IsEmpty(), "There is still something allocated!");
    for (auto& bucket : m_Buckets)
    {
      m_pParent->Deallocate(bucket.GetPtr());
    }
  }

  /// \brief Sets the maximum allocation size that can be handled by this stack allocator. Allocations larger than this size will be directly allocated from the parent allocator.
  EZ_FORCE_INLINE void SetMaxAllocationSize(ezUInt32 uiSize)
  {
    EZ_ASSERT_DEV(m_Allocations.IsEmpty(), "Cannot change max allocation size while there are active allocations!");

    m_uiMaxAllocSizeLog2 = ezMath::Log2i(uiSize);
  }

  EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
  {
    // For allocations larger than the max allocation size, we directly allocate from the parent allocator.
    if (uiSize > GetMaxAllocationSize())
    {
      return m_pParent->Allocate(uiSize, uiAlign);
    }

    EZ_LOCK(m_Mutex);

    EZ_IGNORE_UNUSED(uiAlign);
    EZ_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment {0}", ((ezUInt32)uiAlign));
    const ezUInt32 uiSize32 = static_cast<ezUInt32>(ezMemoryUtils::AlignSize(uiSize, (size_t)Alignment));

    auto bucket = GetOrCreateBucket(uiSize32);
    EZ_ASSERT_DEBUG(m_uiCurrentBucketOffset + uiSize <= bucket.GetCount(), "");

    ezUInt8* ptr = bucket.GetPtr() + m_uiCurrentBucketOffset;

    const ezUInt32 uiGlobalOffset = (m_uiCurrentBucketIndex << m_uiMaxAllocSizeLog2) + m_uiCurrentBucketOffset;
    m_Allocations.PushBack({ptr, uiGlobalOffset, uiSize32});

    m_uiCurrentBucketOffset += uiSize32;

    return ptr;
  }

  EZ_FORCE_INLINE void Deallocate(void* pPtr)
  {
    EZ_LOCK(m_Mutex);

    if (m_Allocations.IsEmpty())
    {
      m_pParent->Deallocate(pPtr);
      return;
    }

    auto& lastAlloc = m_Allocations.PeekBack();
    if (lastAlloc.m_Ptr == pPtr)
    {
      const ezUInt32 uiOffsetMask = ezMath::Bitmask_LowN<ezUInt32>(m_uiMaxAllocSizeLog2);

      m_uiCurrentBucketIndex = static_cast<ezUInt16>(lastAlloc.m_uiGlobalOffset >> m_uiMaxAllocSizeLog2);
      m_uiCurrentBucketOffset = lastAlloc.m_uiGlobalOffset & uiOffsetMask;

      if constexpr (OverwriteMemoryOnFree)
      {
        ezMemoryUtils::PatternFill(static_cast<ezUInt8*>(pPtr), 0xCD, lastAlloc.m_uiSize);
      }

      m_Allocations.PopBack();

      while (m_Allocations.IsEmpty() == false)
      {
        auto& alloc = m_Allocations.PeekBack();
        if (alloc.m_Ptr != nullptr)
          return;

        m_uiCurrentBucketIndex = static_cast<ezUInt16>(alloc.m_uiGlobalOffset >> m_uiMaxAllocSizeLog2);
        m_uiCurrentBucketOffset = alloc.m_uiGlobalOffset & uiOffsetMask;

        m_Allocations.PopBack();
      }

      return;
    }
    else
    {
      for (ezUInt32 i = m_Allocations.GetCount() - 1; i > 0; --i)
      {
        auto& alloc = m_Allocations[i - 1];
        if (alloc.m_Ptr == pPtr)
        {
          if constexpr (OverwriteMemoryOnFree)
          {
            ezMemoryUtils::PatternFill(static_cast<ezUInt8*>(pPtr), 0xCD, alloc.m_uiSize);
          }

          alloc.m_Ptr = nullptr;
          return;
        }
      }
    }

    // Allocation not found, this means it was a large allocation that was directly allocated from the parent allocator.
    m_pParent->Deallocate(pPtr);
  }

  EZ_ALWAYS_INLINE ezAllocator* GetParent() const { return m_pParent; }

private:
  ezUInt32 GetMaxAllocationSize() const { return 1u << m_uiMaxAllocSizeLog2; }

  ezArrayPtr<ezUInt8> GetOrCreateBucket(ezUInt32 uiRequestedSize)
  {
    const ezUInt32 uiMaxAllocSize = GetMaxAllocationSize();
    const bool bFitsIntoCurrentBucket = !m_Buckets.IsEmpty() && m_uiCurrentBucketOffset + uiRequestedSize <= uiMaxAllocSize;
    if (!bFitsIntoCurrentBucket)
    {
      m_uiCurrentBucketIndex = static_cast<ezUInt16>(m_Buckets.GetCount());

      auto newBucket = ezMakeArrayPtr(static_cast<ezUInt8*>(m_pParent->Allocate(uiMaxAllocSize, Alignment)), uiMaxAllocSize);
      m_Buckets.PushBack(newBucket);

      m_uiCurrentBucketOffset = 0;
    }

    return m_Buckets[m_uiCurrentBucketIndex];
  }

  ezAllocator* m_pParent = nullptr;

  ezMutex m_Mutex;

  ezUInt16 m_uiMaxAllocSizeLog2 = 0;
  ezUInt16 m_uiCurrentBucketIndex = 0;
  ezUInt32 m_uiCurrentBucketOffset = 0;

  ezSmallArray<ezArrayPtr<ezUInt8>, 4> m_Buckets;

  struct AllocationInfo
  {
    EZ_DECLARE_POD_TYPE();

    void* m_Ptr;
    ezUInt32 m_uiGlobalOffset;
    ezUInt32 m_uiSize;
  };

  ezSmallArray<AllocationInfo, 16> m_Allocations;
};
