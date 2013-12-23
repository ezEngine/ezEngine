
template <typename A, typename T, typename M>
EZ_FORCE_INLINE ezAllocator<A, T, M>::ezAllocator(const char* szName, ezIAllocator* pParent /* = NULL */) : 
  ezIAllocator(szName),
  m_allocator(pParent),
  m_threadHandle(ezThreadUtils::GetCurrentThreadHandle())
{
}

template <typename A, typename T, typename M>
ezAllocator<A, T, M>::~ezAllocator()
{
  EZ_ASSERT_API(m_threadHandle == ezThreadUtils::GetCurrentThreadHandle(), "Allocator is deleted from another thread");

  DumpMemoryLeaks();
  m_tracker.DumpMemoryLeaks();
}

template <typename A, typename T, typename M>
void* ezAllocator<A, T, M>::Allocate(size_t uiSize, size_t uiAlign)
{
  // zero size allocations always return NULL without tracking (since deallocate NULL is ignored)
  if(uiSize == 0)
    return NULL;

  EZ_ASSERT_API(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  ezLock<M> lock(m_mutex);
  
  void* ptr = m_allocator.Allocate(uiSize, uiAlign);
  EZ_ASSERT(ptr != NULL, "Could not allocate %d bytes. Out of memory?", uiSize);

  m_tracker.AddAllocation(ptr, m_allocator.AllocatedSize(ptr), m_allocator.UsedMemorySize(ptr));

  return ptr;
}

template <typename A, typename T, typename M>
void ezAllocator<A, T, M>::Deallocate(void* ptr)
{
  ezLock<M> lock(m_mutex);

  m_tracker.RemoveAllocation(ptr, m_allocator.AllocatedSize(ptr), m_allocator.UsedMemorySize(ptr));

  m_allocator.Deallocate(ptr);
}

template <typename A, typename T, typename M>
size_t ezAllocator<A, T, M>::AllocatedSize(const void* ptr)
{
  ezLock<M> lock(m_mutex);

  return m_allocator.AllocatedSize(ptr);
}

template <typename A, typename T, typename M>
size_t ezAllocator<A, T, M>::UsedMemorySize(const void* ptr)
{
  ezLock<M> lock(m_mutex);

  return m_allocator.UsedMemorySize(ptr);
}

template <typename A, typename T, typename M>
void ezAllocator<A, T, M>::GetStats(Stats& stats) const
{
  stats.m_uiNumAllocations = m_tracker.GetNumAllocations();
  stats.m_uiNumDeallocations = m_tracker.GetNumDeallocations();
  stats.m_uiNumLiveAllocations = m_tracker.GetNumLiveAllocations();
  stats.m_uiAllocationSize = m_tracker.GetAllocationSize();
  stats.m_uiUsedMemorySize = m_tracker.GetUsedMemorySize();
}

template <typename A, typename T, typename M>
EZ_FORCE_INLINE ezIAllocator* ezAllocator<A, T, M>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, typename T, typename M>
EZ_FORCE_INLINE const T& ezAllocator<A, T, M>::GetTracker() const
{
  return m_tracker;
}

