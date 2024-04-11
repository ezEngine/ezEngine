#if defined(TRACY_ENABLE)
#  include <tracy/tracy/Tracy.hpp>
#endif

EZ_FORCE_INLINE void* ezAllocPolicyAlignedHeap::Allocate(size_t uiSize, size_t uiAlign)
{
  // alignment has to be at least sizeof(void*) otherwise posix_memalign will fail
  uiAlign = ezMath::Max<size_t>(uiAlign, 16u);

  void* ptr = nullptr;

  int res = posix_memalign(&ptr, uiAlign, uiSize);
  EZ_IGNORE_UNUSED(res);
  EZ_ASSERT_DEV(res == 0, "posix_memalign failed with error: {0}", res);

  EZ_CHECK_ALIGNMENT(ptr, uiAlign);

#if defined(TRACY_ENABLE)
  TracyAlloc(ptr, uiSize);
#endif

  return ptr;
}

EZ_ALWAYS_INLINE void ezAllocPolicyAlignedHeap::Deallocate(void* ptr)
{
#if defined(TRACY_ENABLE)
  TracyFree(pPtr);
#endif

  free(ptr);
}
