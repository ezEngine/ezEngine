#pragma once

#include <Foundation/Containers/HybridArray.h>

namespace ezMemoryPolicies
{
  /// \brief This allocation policy works like a stack. You can only "push" and "pop" allocations
  ///   in the correct order.
  ///
  /// \note It is also possible to free all allocations at once.
  ///
  /// \see ezAllocator
  class ezStackAllocation
  {
  public:
    enum
    {
      Alignment = sizeof(void*)
    };

    EZ_FORCE_INLINE ezStackAllocation(ezAllocatorBase* pParent)
      : m_pParent(pParent),
      m_uiCurrentBucketIndex(0),
      m_uiCurrentBucketSize(4096),
      m_pNextAllocation(nullptr) {}

    EZ_FORCE_INLINE ~ezStackAllocation()
    {
      EZ_ASSERT_DEV(m_uiCurrentBucketIndex == 0 && m_pNextAllocation == m_currentBucket.GetPtr(),
        "There is still something allocated!");
      for (auto& bucket : m_buckets)
      {
        EZ_DELETE_ARRAY(m_pParent, bucket.memory);
      }
    }

    EZ_FORCE_INLINE void SetNextBucketSize(ezUInt32 uiSize)
    {
      m_uiCurrentBucketSize = uiSize;
    }

    EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      EZ_ASSERT_DEV(uiAlign <= Alignment && Alignment % uiAlign == 0, "Unsupported alignment %d", uiAlign);
      uiSize = ezMemoryUtils::AlignSize(uiSize, (size_t)Alignment);

      // Do we need a new bucket?
      if (m_pNextAllocation + uiSize > m_currentBucket.GetPtr() + m_currentBucket.GetCount())
      {
        if (m_currentBucket.GetPtr() != nullptr)
        {
          m_buckets[m_uiCurrentBucketIndex].pLastAllocation = m_pNextAllocation;
          m_uiCurrentBucketIndex++;
        }
        while (uiSize > m_uiCurrentBucketSize)
          m_uiCurrentBucketSize *= 2;

        // Is there still a not yet freed bucket?
        if (m_uiCurrentBucketIndex < m_buckets.GetCount())
        {
          // is the allocation to big for the current bucket?
          while (m_uiCurrentBucketIndex < m_buckets.GetCount() &&
            uiSize > m_buckets[m_uiCurrentBucketIndex].memory.GetCount())
          {
            m_uiCurrentBucketIndex++;
          }
          if (m_uiCurrentBucketIndex >= m_buckets.GetCount())
            goto AllocNewBucket;
          else
            m_currentBucket = m_buckets[m_uiCurrentBucketIndex].memory;
        }
        else
        {
          AllocNewBucket:
          m_currentBucket = EZ_NEW_ARRAY(m_pParent, ezUInt8, m_uiCurrentBucketSize);
          m_buckets.ExpandAndGetRef().memory = m_currentBucket;
          m_uiCurrentBucketSize *= 2;
        }
        m_pNextAllocation = m_currentBucket.GetPtr();
      }
      EZ_ASSERT_DEBUG(m_pNextAllocation + uiSize <= m_currentBucket.GetPtr() + m_currentBucket.GetCount(), "");

      auto pResult = m_pNextAllocation;
      m_pNextAllocation += uiSize;
      return pResult;
    }

    EZ_FORCE_INLINE void Deallocate(void* ptr)
    {
      // ptr is not on top of the stack, can't deallocate now
      if (ptr < m_currentBucket.GetPtr() || ptr > m_pNextAllocation)
        return;

      // does this empty the current bucket?
      if (ptr == m_currentBucket.GetPtr())
      {
        m_pNextAllocation = (ezUInt8*)ptr;
        m_buckets[m_uiCurrentBucketIndex].pLastAllocation = (ezUInt8*)ptr;
        while (m_pNextAllocation == m_currentBucket.GetPtr() && m_uiCurrentBucketIndex > 0)
        {
          m_uiCurrentBucketIndex--;
          Bucket& currentBucket = m_buckets[m_uiCurrentBucketIndex];
          m_currentBucket = currentBucket.memory;
          m_pNextAllocation = currentBucket.pLastAllocation;
        }
      }
      else
      {
        m_pNextAllocation = (ezUInt8*)ptr;
      }
    }

    EZ_FORCE_INLINE void Reset()
    {
      for (auto& bucket : m_buckets)
      {
        bucket.pLastAllocation = bucket.memory.GetPtr();
      }
      m_uiCurrentBucketIndex = 0;

      if (!m_buckets.IsEmpty())
        m_currentBucket = m_buckets[0].memory;
      else
        m_currentBucket.Reset();

      m_pNextAllocation = m_currentBucket.GetPtr();
    }

    EZ_FORCE_INLINE ezAllocatorBase* GetParent() const { return m_pParent; }

  private:
    struct Bucket
    {
      ezArrayPtr<ezUInt8> memory;
      ezUInt8* pLastAllocation;
    };

    ezAllocatorBase* m_pParent;
    ezUInt32 m_uiCurrentBucketIndex;
    ezUInt32 m_uiCurrentBucketSize;
    ezArrayPtr<ezUInt8> m_currentBucket;
    ezUInt8* m_pNextAllocation;
    ezHybridArray<Bucket, 4> m_buckets;
  };
}
