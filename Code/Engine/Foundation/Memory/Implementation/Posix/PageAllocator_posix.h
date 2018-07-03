
// static
void* ezPageAllocator::AllocatePage(size_t uiSize)
{
  void* ptr = nullptr;
  size_t uiAlign = ezSystemInformation::Get().GetMemoryPageSize();
  const int res = posix_memalign(&ptr, uiAlign, uiSize);
  EZ_ASSERT_DEBUG(res == 0, "Failed to align pointer");

  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

  ezMemoryTracker::AddAllocation(GetPageAllocatorId(), ptr, uiSize, uiAlign);

  return ptr;
}

// static
void ezPageAllocator::DeallocatePage(void* ptr)
{
  ezMemoryTracker::RemoveAllocation(GetPageAllocatorId(), ptr);

  free(ptr);
}
