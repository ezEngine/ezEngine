#include <unistd.h>

inline ezPageAllocation::ezPageAllocation()
{
}

inline ezPageAllocation::~ezPageAllocation()
{
}

inline void* ezPageAllocation::PageAllocate(size_t uiSize)
{
  void* pMemory = NULL;
  
  posix_memalign(&pMemory, sysconf(_SC_PAGESIZE), uiSize);

  return pMemory;
}

inline void ezPageAllocation::PageDeallocate(void* ptr)
{
  if (ptr != NULL)
    free(ptr);
}
