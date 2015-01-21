
EZ_FORCE_INLINE void* ezAlignedHeapAllocation::Allocate(size_t uiSize, size_t uiAlign)
{
  // alignment has to be at least sizeof(void*) otherwise posix_memalign will fail
  uiAlign = ezMath::Max(uiAlign, sizeof(void*));
  
  void* ptr = nullptr;
  
  int res = posix_memalign(&ptr, uiAlign, uiSize);
  EZ_IGNORE_UNUSED(res);
  EZ_ASSERT_DEV(res == 0, "posix_memalign failed with error: %d", res);

  EZ_CHECK_ALIGNMENT(ptr, uiAlign);
  
  return ptr;
}

EZ_FORCE_INLINE void ezAlignedHeapAllocation::Deallocate(void* ptr)
{
  free(ptr);
}

