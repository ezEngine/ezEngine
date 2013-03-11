
inline ezHeapAllocation::ezHeapAllocation(ezIAllocator* pParent)
{
}

inline ezHeapAllocation::~ezHeapAllocation()
{
}

inline void* ezHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  void* ptr = malloc(uiSize);
  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

inline void ezHeapAllocation::Deallocate(void* ptr)
{
  free(ptr);
}

inline size_t ezHeapAllocation::AllocatedSize(const void* ptr)
{
  return _msize(const_cast<void*>(ptr));
}

inline size_t ezHeapAllocation::UsedMemorySize(const void* ptr)
{
  return _msize(const_cast<void*>(ptr));
}
