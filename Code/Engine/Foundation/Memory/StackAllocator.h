#pragma once

#include <Foundation/Memory/Allocator.h>
#include <Foundation/Memory/Policies/StackAllocation.h>

template <ezUInt32 TrackingFlags = ezMemoryTrackingFlags::Default>
class ezStackAllocator : 
    public ezAllocator<ezMemoryPolicies::ezStackAllocation,
                       TrackingFlags>
{
public:
  ezStackAllocator(const char* szName, ezAllocatorBase* pParent);

  /// \brief
  ///   Resets the allocator freeing all memory.
  ///
  /// \note This function does not call any destructors, 
  ///    only use this when POD types have been allocated.
  void Reset();
};

#include <Foundation/Memory/Implementation/StackAllocator_inl.h>
