
template <size_t uiAlignment>
ezAlignedHeapAllocation<uiAlignment>::ezAlignedHeapAllocation(ezIAllocator* pParent)
{
}

template <size_t uiAlignment>
ezAlignedHeapAllocation<uiAlignment>::~ezAlignedHeapAllocation()
{
}

template <size_t uiAlignment>
void* ezAlignedHeapAllocation<uiAlignment>::Allocate(size_t uiSize, size_t uiAlign)
{
  EZ_ASSERT_API(uiAlign <= uiAlignment, "Alignment is too big. Max %d bytes alignment are allowed, but %d bytes alignment requested",
                uiAlignment, uiAlign);
  
  void* ptr = NULL;
  
  if(posix_memalign(&ptr, uiAlignment, uiSize) != 0)
    return NULL;
  
  EZ_CHECK_ALIGNMENT(ptr, uiAlign);
  
  return ptr;
}

template <size_t uiAlignment>
void ezAlignedHeapAllocation<uiAlignment>::Deallocate(void* ptr)
{
  free(ptr);
}

template <size_t uiAlignment>
size_t ezAlignedHeapAllocation<uiAlignment>::AllocatedSize(const void* ptr)
{
  return 0;
}

template <size_t uiAlignment>
size_t ezAlignedHeapAllocation<uiAlignment>::UsedMemorySize(const void* ptr)
{
  return 0;
}
