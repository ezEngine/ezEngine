#include <Foundation/FoundationPCH.h>

#include <Foundation/Memory/MemoryTracker.h>
#include <Foundation/Memory/PageAllocator.h>

ezAllocatorId ezPageAllocator::GetId()
{
  static ezAllocatorId id;

  if (id.IsInvalidated())
  {
    id = ezMemoryTracker::RegisterAllocator("Page", ezMemoryTrackingFlags::Default, ezAllocatorId());
  }

  return id;
}


