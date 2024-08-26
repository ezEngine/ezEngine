#pragma once

#include <Foundation/Basics.h>

/// \brief This Allocation policy redirects all operations to its parent.
///
/// \note Note that the stats are taken on the proxy as well as on the parent.
///
/// \see ezAllocatorWithPolicy
class ezAllocPolicyProxy
{
public:
  EZ_FORCE_INLINE ezAllocPolicyProxy(ezAllocator* pParent)
    : m_pParent(pParent)
  {
    EZ_ASSERT_ALWAYS(m_pParent != nullptr, "Parent allocator must not be nullptr");
  }

  EZ_FORCE_INLINE void* Allocate(size_t uiSize, size_t uiAlign) { return m_pParent->Allocate(uiSize, uiAlign); }

  EZ_FORCE_INLINE void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
  {
    return m_pParent->Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);
  }

  EZ_FORCE_INLINE void Deallocate(void* pPtr) { m_pParent->Deallocate(pPtr); }

  EZ_FORCE_INLINE size_t AllocatedSize(const void* pPtr) { return m_pParent->AllocatedSize(pPtr); }

  EZ_ALWAYS_INLINE ezAllocator* GetParent() const { return m_pParent; }

private:
  ezAllocator* m_pParent;
};
