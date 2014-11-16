#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorBase.h>
#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Threading/ThreadUtils.h>

EZ_MAKE_MEMBERFUNCTION_CHECKER(Reallocate, ezHasReallocate);

#include <Foundation/Memory/Implementation/Allocator_inl.h>

/// \brief Policy based allocator implementation of the ezAllocatorBase interface.
///
/// AllocationPolicy defines how the actual memory is allocated.\n
/// TrackingFlags defines how stats about allocations are tracked.\n
template <typename AllocationPolicy, ezUInt32 TrackingFlags = ezMemoryTrackingFlags::Default>
class ezAllocator : 
  public ezInternal::ezAllocatorMixinReallocate<AllocationPolicy,
                                                TrackingFlags, 
                                                ezHasReallocate<AllocationPolicy, 
                                                                void*(AllocationPolicy::*)(void*, size_t, size_t, size_t)
                                                               >::value
                                               >
{
public:
    ezAllocator(const char* szName, ezAllocatorBase* pParent = nullptr)
      : ezInternal::ezAllocatorMixinReallocate<AllocationPolicy,
                                               TrackingFlags,
                                               ezHasReallocate<AllocationPolicy,
                                                               void*(AllocationPolicy::*)(void*, size_t, size_t, size_t)
                                                              >::value
                                              >(szName, pParent)
    {
    }
};