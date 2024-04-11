
EZ_FORCE_INLINE void* ezAllocPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  uiAlign = ezMath::Max<size_t>(uiAlign, 16u);

  void* ptr = _aligned_malloc(uiSize, uiAlign);
  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

  return ptr;
}

EZ_ALWAYS_INLINE void ezAllocPolicyAlignedHeap::Deallocate(void* pPtr)
{
  _aligned_free(pPtr);
}
