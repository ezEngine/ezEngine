#pragma once

#include <Foundation/Containers/HybridArray.h>

namespace ezMemoryPolicies
{
  /// \brief This allocation policy works like a stack. You can only "push" and "pop" allocations
  ///   in the correct order.
  ///
  /// \note It is also possible to free all allocations at once.
  ///
  /// \see ezAllocatorWithPolicy
  class ezAllocPolicyStack
  {
  public:
    enum
    {
      Alignment = 16
    };

    EZ_FORCE_INLINE ezAllocPolicyStack(ezAllocator* pParent)
      : m_pParent(pParent)
      , m_uiNextBucketSize(4096)
    {
    }

    EZ_FORCE_INLINE ~ezAllocPolicyStack()
    {
      EZ_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && (m_Buckets.IsEmpty() || m_Buckets[m_uiCurrentBucketIndex].GetPtr() == m_pNextAllocation),
        "There is still something allocated!");
      for (auto& bucket : m_Buckets)
      {
        m_pParent->Deallocate(bucket.GetPtr());
      }
    }

    EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      EZ_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment {0}", ((ezUInt32)uiAlign));
      uiSize = ezMemoryUtils::AlignSize(uiSize, (size_t)Alignment);

      bool bFoundBucket = !m_Buckets.IsEmpty() && m_pNextAllocation + uiSize <= m_Buckets[m_uiCurrentBucketIndex].GetEndPtr();

      if (!bFoundBucket)
      {
        // Check if there is an empty bucket that fits the allocation
        for (ezUInt32 i = m_uiCurrentBucketIndex + 1; i < m_Buckets.GetCount(); ++i)
        {
          auto& testBucket = m_Buckets[i];
          if (uiSize <= testBucket.GetCount())
          {
            m_uiCurrentBucketIndex = i;
            m_pNextAllocation = testBucket.GetPtr();
            bFoundBucket = true;
            break;
          }
        }
      }

      if (!bFoundBucket)
      {
        while (uiSize > m_uiNextBucketSize)
        {
          EZ_ASSERT_DEBUG(m_uiNextBucketSize > 0, "");

          m_uiNextBucketSize *= 2;
        }

        m_uiCurrentBucketIndex = m_Buckets.GetCount();

        auto newBucket = ezArrayPtr<ezUInt8>(static_cast<ezUInt8*>(m_pParent->Allocate(m_uiNextBucketSize, Alignment)), m_uiNextBucketSize);
        m_Buckets.PushBack(newBucket);

        m_pNextAllocation = newBucket.GetPtr();

        m_uiNextBucketSize *= 2;
      }

      EZ_ASSERT_DEBUG(m_pNextAllocation + uiSize <= m_Buckets[m_uiCurrentBucketIndex].GetEndPtr(), "");

      ezUInt8* ptr = m_pNextAllocation;
      m_pNextAllocation += uiSize;
      return ptr;
    }

    EZ_FORCE_INLINE void Deallocate(void* pPtr)
    {
      // Individual deallocation is not supported by this allocator
    }

    EZ_FORCE_INLINE void Reset()
    {
      m_uiCurrentBucketIndex = 0;
      m_pNextAllocation = !m_Buckets.IsEmpty() ? m_Buckets[0].GetPtr() : nullptr;
    }

    EZ_FORCE_INLINE void FillStats(ezAllocator::Stats& ref_stats)
    {
      ref_stats.m_uiNumAllocations = m_Buckets.GetCount();
      for (auto& bucket : m_Buckets)
      {
        ref_stats.m_uiAllocationSize += bucket.GetCount();
      }
    }

    EZ_ALWAYS_INLINE ezAllocator* GetParent() const { return m_pParent; }

  private:
    ezAllocator* m_pParent = nullptr;

    ezUInt32 m_uiCurrentBucketIndex = 0;
    ezUInt32 m_uiNextBucketSize = 0;

    ezUInt8* m_pNextAllocation = nullptr;

    ezHybridArray<ezArrayPtr<ezUInt8>, 4> m_Buckets;
  };
} // namespace ezMemoryPolicies
