#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Memory/IAllocator.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/ThreadUtils.h>

/// policy based allocator implementation
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
