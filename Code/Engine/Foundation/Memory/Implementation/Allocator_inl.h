
template <typename A, ezUInt32 TrackingFlags>
EZ_FORCE_INLINE ezAllocator<A, TrackingFlags>::ezAllocator(const char* szName, ezAllocatorBase* pParent /* = NULL */) : 
  m_allocator(pParent),
  m_threadHandle(ezThreadUtils::GetCurrentThreadHandle())
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    EZ_CHECK_AT_COMPILETIME_MSG((TrackingFlags & ~ezMemoryTrackingFlags::All) == 0, "Invalid tracking flags");
    const ezUInt32 uiTrackingFlags = TrackingFlags;
    ezBitflags<ezMemoryTrackingFlags> flags = *reinterpret_cast<const ezBitflags<ezMemoryTrackingFlags>*>(&uiTrackingFlags);
    m_Id = ezMemoryTracker::RegisterAllocator(szName, flags);
  }
}

template <typename A, ezUInt32 TrackingFlags>
ezAllocator<A, TrackingFlags>::~ezAllocator()
{
  EZ_ASSERT_API(m_threadHandle == ezThreadUtils::GetCurrentThreadHandle(), "Allocator is deleted from another thread");

  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::DeregisterAllocator(m_Id);
  }
}

template <typename A, ezUInt32 TrackingFlags>
void* ezAllocator<A, TrackingFlags>::Allocate(size_t uiSize, size_t uiAlign)
{
  // zero size allocations always return NULL without tracking (since deallocate NULL is ignored)
  if (uiSize == 0)
    return NULL;

  EZ_ASSERT_API(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  void* ptr = m_allocator.Allocate(uiSize, uiAlign);
  EZ_ASSERT(ptr != NULL, "Could not allocate %d bytes. Out of memory?", uiSize);

  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::AddAllocation(m_Id, ptr, uiSize, uiAlign);
  }

  return ptr;
}

template <typename A, ezUInt32 TrackingFlags>
void ezAllocator<A, TrackingFlags>::Deallocate(void* ptr)
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::RemoveAllocation(m_Id, ptr);
  }

  m_allocator.Deallocate(ptr);
}

template <typename A, ezUInt32 TrackingFlags>
size_t ezAllocator<A, TrackingFlags>::AllocatedSize(const void* ptr)
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    return ezMemoryTracker::GetAllocationInfo(m_Id, ptr).m_uiSize;
  }

  return 0;
}

template <typename A, ezUInt32 TrackingFlags>
ezAllocatorBase::Stats ezAllocator<A, TrackingFlags>::GetStats() const
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    return ezMemoryTracker::GetAllocatorStats(m_Id);
  }

  return Stats();
}

template <typename A, ezUInt32 TrackingFlags>
EZ_FORCE_INLINE ezAllocatorBase* ezAllocator<A, TrackingFlags>::GetParent() const
{
  return m_allocator.GetParent();
}

