#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/IAllocator.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/ThreadUtils.h>

/// \brief Policy based allocator implementation of the ezIAllocator interface.
///
/// This class allows the creation of custom allocators by combining different policies, e.g.
/// a custom allocation algorithm can be combined with the ezStackTracking policy to add stack traces for memory leaks. \n
/// \n
/// AllocationPolicy defines how the actual memory is allocated.\n
/// BoundsCheckingPolicy defines if and how to add guard checking around the memory block to detect buffer over- and underruns.\n
/// TrackingPolicy defines how stats about allocations are tracked.\n
/// MutexType defines if a locking mechanism should prevent multiple threads from entering the methods simultaneously.
template <typename AllocationPolicy, typename BoundsCheckingPolicy, typename TrackingPolicy, 
  typename MutexType>
class ezAllocator : public ezIAllocator
{
public:
  ezAllocator(const char* szName, ezIAllocator* pParent = NULL);
  ~ezAllocator();

  ezIAllocator* GetParent() const EZ_OVERRIDE;

  const TrackingPolicy& GetTracker() const;

private:
  AllocationPolicy m_allocator;
  BoundsCheckingPolicy m_boundsChecker;
  TrackingPolicy m_tracker;
  MutexType m_mutex;

  ezThreadHandle m_threadHandle;

  void* Allocate(size_t uiSize, size_t uiAlign) EZ_OVERRIDE;
  void Deallocate(void* ptr) EZ_OVERRIDE;
  size_t AllocatedSize(const void* ptr) EZ_OVERRIDE;
  size_t UsedMemorySize(const void* ptr) EZ_OVERRIDE;
  void GetStats(Stats& stats) EZ_OVERRIDE;
};

#include <Foundation/Memory/Implementation/Allocator_inl.h>
