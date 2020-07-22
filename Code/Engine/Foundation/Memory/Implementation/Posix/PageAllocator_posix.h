
#include <Foundation/Time/Time.h>

// static
void* ezPageAllocator::AllocatePage(size_t uiSize)
{
  ezTime fAllocationTime = ezTime::Now();

  void* ptr = nullptr;
  size_t uiAlign = ezSystemInformation::Get().GetMemoryPageSize();
  const int res = posix_memalign(&ptr, uiAlign, uiSize);
  EZ_ASSERT_DEBUG(res == 0, "Failed to align pointer");
  EZ_IGNORE_UNUSED(res);

  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

  ezMemoryTracker::AddAllocation(GetPageAllocatorId(), ezMemoryTrackingFlags::Default, ptr, uiSize, uiAlign, ezTime::Now() - fAllocationTime);

  return ptr;
}

// static
void ezPageAllocator::DeallocatePage(void* ptr)
{
  ezMemoryTracker::RemoveAllocation(GetPageAllocatorId(), ptr);

  free(ptr);
}
