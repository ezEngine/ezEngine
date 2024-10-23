#pragma once

#include <Foundation/Basics.h>

/// \brief Aligned Heap memory allocation policy.
///
/// \see ezAllocatorWithPolicy
class ezAllocPolicyAlignedHeap
{
public:
  EZ_ALWAYS_INLINE ezAllocPolicyAlignedHeap(ezAllocator* pParent) { EZ_IGNORE_UNUSED(pParent); }
  EZ_ALWAYS_INLINE ~ezAllocPolicyAlignedHeap() = default;

  void* Allocate(size_t uiSize, size_t uiAlign);
  void Deallocate(void* pPtr);

  EZ_ALWAYS_INLINE ezAllocator* GetParent() const { return nullptr; }
};

// include the platform specific implementation
#include <AllocPolicyAlignedHeap_Platform.inl>
