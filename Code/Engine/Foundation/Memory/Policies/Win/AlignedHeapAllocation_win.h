
EZ_FORCE_INLINE void* ezAlignedHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  void* ptr = _aligned_malloc(uiSize, uiAlign);
  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

EZ_FORCE_INLINE void ezAlignedHeapAllocation::Deallocate(void* ptr)
{
  _aligned_free(ptr);
}


