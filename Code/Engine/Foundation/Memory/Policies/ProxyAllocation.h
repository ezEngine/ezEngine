#pragma once

#include <Foundation/Basics.h>

namespace ezMemoryPolicies
{
  /// \brief This Allocation policy redirects all operations to its parent.
  ///
  /// \note Note that the stats are taken on the proxy as well as on the parent.
  ///
  /// \see ezAllocator
  class ezProxyAllocation
  {
  public:
    EZ_FORCE_INLINE ezProxyAllocation(ezAllocatorBase* pParent) :
      m_pParent(pParent)
    {
      EZ_ASSERT_ALWAYS(m_pParent != nullptr, "Parent allocator must not be nullptr");
    }

    EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign)
    {
      return m_pParent->Allocate(uiSize, uiAlign);
    }

    EZ_FORCE_INLINE void* Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
    {
      return m_pParent->Reallocate(ptr, uiCurrentSize, uiNewSize, uiAlign);
    }

    EZ_FORCE_INLINE void Deallocate(void* ptr)
    {
      m_pParent->Deallocate(ptr);
    }

    EZ_FORCE_INLINE size_t AllocatedSize(const void* ptr)
    {
      return m_pParent->AllocatedSize(ptr);
    }

    EZ_FORCE_INLINE ezAllocatorBase* GetParent() const { return m_pParent; }

  private:
    ezAllocatorBase* m_pParent;
  };
}

