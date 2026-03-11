#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_MAKE_MEMBERFUNCTION_CHECKER(Reallocate, ezHasReallocate);

#include <Foundation/Memory/Implementation/AllocatorMixin_inl.h>

/// \brief Policy-based allocator that combines allocation strategies with tracking modes.
///
/// AllocationPolicy defines how the actual memory is allocated.\n
/// TrackingFlags defines how stats about allocations are tracked.\n
template <typename AllocationPolicy, ezAllocatorTrackingMode TrackingMode = ezAllocatorTrackingMode::Default>
class ezAllocatorWithPolicy : public ezInternal::ezAllocatorMixinReallocate<AllocationPolicy, TrackingMode,
                                ezHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>
{
public:
  ezAllocatorWithPolicy(ezStringView sName, ezAllocator* pParent = nullptr)
    : ezInternal::ezAllocatorMixinReallocate<AllocationPolicy, TrackingMode,
        ezHasReallocate<AllocationPolicy, void* (AllocationPolicy::*)(void*, size_t, size_t, size_t)>::value>(sName, pParent)
  {
  }
};
