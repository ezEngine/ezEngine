
template <typename A, typename B, typename T, typename M>
EZ_FORCE_INLINE ezAllocator<A, B, T, M>::ezAllocator(const char* szName, ezIAllocator* pParent /* = NULL */) : 
  ezIAllocator(szName),
  m_allocator(pParent),
  m_threadHandle(ezThreadUtils::GetCurrentThreadHandle())
{
}

template <typename A, typename B, typename T, typename M>
ezAllocator<A, B, T, M>::~ezAllocator()
{
  EZ_ASSERT_API(m_threadHandle == ezThreadUtils::GetCurrentThreadHandle(), "Allocator is deleted from another thread");

  if (m_tracker.GetAllocationSize() != 0 || m_tracker.GetNumLiveAllocations() != 0)
  {
    EZ_REPORT_FAILURE("Memory leaks found");
  
    #if EZ_PLATFORM_WINDOWS

      // todo: make this platform independent
      wchar_t szName[32];
      mbstowcs(szName, m_szName, 32);
      wchar_t szBuffer[512];
      swprintf_s(szBuffer,
        L"\n--------------------------------------------------------------------\n"
        L"Allocator '%s' has leaks: %d bytes in %d allocation(s)"
        L"\n--------------------------------------------------------------------\n\n",
        szName, (int)m_tracker.GetAllocationSize(), (int)m_tracker.GetNumLiveAllocations());
    
      OutputDebugStringW(szBuffer);
  
    #endif

    m_tracker.DumpMemoryLeaks();
  }
}

template <typename A, typename B, typename T, typename M>
void* ezAllocator<A, B, T, M>::Allocate(size_t uiSize, size_t uiAlign)
{
  EZ_ASSERT_API(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  ezLock<M> lock(m_mutex);
  
  const size_t uiGuardSizeFront = B::GuardSizeFront ? ezMath::Max<size_t>(B::GuardSizeFront, uiAlign) : 0;
  const size_t uiGuardedSize = uiSize + uiGuardSizeFront + B::GuardSizeBack;

  ezUInt8* pMemory = static_cast<ezUInt8*>(m_allocator.Allocate(uiGuardedSize, uiAlign));

  m_boundsChecker.GuardFront(pMemory, uiGuardSizeFront);
  m_boundsChecker.GuardBack(pMemory + uiGuardedSize);

  void* ptr = pMemory + uiGuardSizeFront;
  m_tracker.AddAllocation(ptr, uiSize, m_allocator.UsedMemorySize(pMemory));

  return ptr;
}

template <typename A, typename B, typename T, typename M>
void ezAllocator<A, B, T, M>::Deallocate(void* ptr)
{
  ezLock<M> lock(m_mutex);

  ezUInt8* ptrAsByte = static_cast<ezUInt8*>(ptr);
  size_t uiOffset = m_boundsChecker.GetGuardOffset(ptr);
  ezUInt8* pMemory = ptrAsByte - uiOffset;

  const size_t uiGuardedSize = m_allocator.AllocatedSize(pMemory);
  const size_t uiAllocatedSize = uiGuardedSize - uiOffset - B::GuardSizeBack;
  const size_t uiUsedMemorySize = m_allocator.UsedMemorySize(pMemory);

  m_boundsChecker.CheckFront(pMemory, uiOffset);
  m_boundsChecker.CheckBack(pMemory + uiGuardedSize);
  
  m_tracker.RemoveAllocation(ptr, uiAllocatedSize, uiUsedMemorySize);

  m_allocator.Deallocate(pMemory);
}

template <typename A, typename B, typename T, typename M>
size_t ezAllocator<A, B, T, M>::AllocatedSize(const void* ptr)
{
  ezLock<M> lock(m_mutex);

  const ezUInt8* pMemory = static_cast<const ezUInt8*>(ptr);
  size_t uiOffset = m_boundsChecker.GetGuardOffset(ptr);

  return m_allocator.AllocatedSize(pMemory - uiOffset) - uiOffset - B::GuardSizeBack;
}

template <typename A, typename B, typename T, typename M>
size_t ezAllocator<A, B, T, M>::UsedMemorySize(const void* ptr)
{
  ezLock<M> lock(m_mutex);

  const ezUInt8* pMemory = static_cast<const ezUInt8*>(ptr);
  size_t uiOffset = m_boundsChecker.GetGuardOffset(ptr);

  return m_allocator.UsedMemorySize(pMemory - uiOffset);
}

template <typename A, typename B, typename T, typename M>
void ezAllocator<A, B, T, M>::GetStats(Stats& stats)
{
  stats.m_uiNumAllocations = m_tracker.GetNumAllocations();
  stats.m_uiNumDeallocations = m_tracker.GetNumDeallocations();
  stats.m_uiNumLiveAllocations = m_tracker.GetNumLiveAllocations();
  stats.m_uiAllocationSize = m_tracker.GetAllocationSize();
  stats.m_uiUsedMemorySize = m_tracker.GetUsedMemorySize();
}

template <typename A, typename B, typename T, typename M>
EZ_FORCE_INLINE ezIAllocator* ezAllocator<A, B, T, M>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, typename B, typename T, typename M>
EZ_FORCE_INLINE const T& ezAllocator<A, B, T, M>::GetTracker() const
{
  return m_tracker;
}
