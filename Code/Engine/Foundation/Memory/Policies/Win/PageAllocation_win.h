
inline ezPageAllocation::ezPageAllocation()
{
}

inline ezPageAllocation::~ezPageAllocation()
{
}

inline void* ezPageAllocation::PageAllocate(size_t uiSize)
{
  void* ptr = ::VirtualAlloc(NULL, uiSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  EZ_ASSERT(ptr != NULL, "Could not allocate memory pages. Error Code '%d'", ::GetLastError());
  return ptr;
}

inline void ezPageAllocation::PageDeallocate(void* ptr)
{
  EZ_VERIFY(::VirtualFree(ptr, 0, MEM_RELEASE), "Could not free memory pages. Error Code '%d'", ::GetLastError());
}
