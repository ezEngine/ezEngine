
EZ_FORCE_INLINE ezHeapAllocation::ezHeapAllocation(ezIAllocator* pParent)
{
}

EZ_FORCE_INLINE ezHeapAllocation::~ezHeapAllocation()
{
}

EZ_FORCE_INLINE void* ezHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  void* ptr = malloc(uiSize);
  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

EZ_FORCE_INLINE void ezHeapAllocation::Deallocate(void* ptr)
{
  free(ptr);
}

EZ_FORCE_INLINE size_t ezHeapAllocation::AllocatedSize(const void* ptr)
{
  return 0;
}

EZ_FORCE_INLINE size_t ezHeapAllocation::UsedMemorySize(const void* ptr)
{
  return 0;
}
