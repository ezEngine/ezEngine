#if defined(TRACY_ENABLE)
#  include <tracy/tracy/Tracy.hpp>
#endif

EZ_FORCE_INLINE void* ezAllocPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  uiAlign = ezMath::Max<size_t>(uiAlign, 16u);

  void* ptr = _aligned_malloc(uiSize, uiAlign);
  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

#if defined(TRACY_ENABLE)
  TracyAlloc(ptr, uiSize);
#endif

  return ptr;
}

EZ_ALWAYS_INLINE void ezAllocPolicyAlignedHeap::Deallocate(void* pPtr)
{
#if defined(TRACY_ENABLE)
  TracyFree(pPtr);
#endif

  _aligned_free(pPtr);
}
