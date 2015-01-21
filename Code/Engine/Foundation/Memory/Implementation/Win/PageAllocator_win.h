
// static
void* ezPageAllocator::AllocatePage(size_t uiSize)
{
  void* ptr = ::VirtualAlloc(nullptr, uiSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  EZ_ASSERT_DEV(ptr != nullptr, "Could not allocate memory pages. Error Code '%d'", ::GetLastError());

  size_t uiAlign = ezSystemInformation::Get().GetMemoryPageSize();
  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

  ezMemoryTracker::AddAllocation(GetPageAllocatorId(), ptr, uiSize, uiAlign);

  return ptr;
}

// static
void ezPageAllocator::DeallocatePage(void* ptr)
{
  ezMemoryTracker::RemoveAllocation(GetPageAllocatorId(), ptr);

  EZ_VERIFY(::VirtualFree(ptr, 0, MEM_RELEASE), "Could not free memory pages. Error Code '%d'", ::GetLastError());
}

