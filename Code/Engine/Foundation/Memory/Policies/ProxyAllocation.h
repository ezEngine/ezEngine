#pragma once

#include <Foundation/Basics.h>

namespace ezMemoryPolicies
{
  /// This Allocation policy redirects all operations to its parent. 
  /// Note that the stats are taken on the proxy as well as on the parent
  class ezProxyAllocation
  {
  public:
    EZ_FORCE_INLINE ezProxyAllocation(ezIAllocator* pParent) :
      m_pParent(pParent)
    {
      EZ_ASSERT_ALWAYS(m_pParent != NULL, "Parent allocator must not be NULL");
    }

    EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      return m_pParent->Allocate(uiSize, uiAlign);
    }

    EZ_FORCE_INLINE void Deallocate(void* ptr)
    {
      m_pParent->Deallocate(ptr);
    }

    EZ_FORCE_INLINE size_t AllocatedSize(const void* ptr)
    {
      return m_pParent->AllocatedSize(ptr);
    }

    EZ_FORCE_INLINE size_t UsedMemorySize(const void* ptr)
    {
      return m_pParent->UsedMemorySize(ptr);
    }

    EZ_FORCE_INLINE ezIAllocator* GetParent() const { return m_pParent; }

  private:
    ezIAllocator* m_pParent;
  };
}
