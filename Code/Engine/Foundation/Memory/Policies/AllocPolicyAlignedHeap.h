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

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
#  include <Foundation/Memory/Policies/Win/AllocPolicyAlignedHeap_win.h>
#elif EZ_ENABLED(EZ_PLATFORM_OSX) || EZ_ENABLED(EZ_PLATFORM_LINUX) || EZ_ENABLED(EZ_PLATFORM_ANDROID)
#  include <Foundation/Memory/Policies/Posix/AllocPolicyAlignedHeap_posix.h>
#else
#  error "ezAllocPolicyAlignedHeap is not implemented on current platform"
#endif
