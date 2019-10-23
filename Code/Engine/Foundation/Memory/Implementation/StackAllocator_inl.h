template <ezUInt32 TrackingFlags>
ezStackAllocator<TrackingFlags>::ezStackAllocator(const char* szName, ezAllocatorBase* pParent)
    : ezAllocator<ezMemoryPolicies::ezStackAllocation, TrackingFlags>(szName, pParent)
    , m_DestructData(pParent)
    , m_PtrToDestructDataIndexTable(pParent)
{
}

template <ezUInt32 TrackingFlags>
ezStackAllocator<TrackingFlags>::~ezStackAllocator()
{
  Reset();
}

template <ezUInt32 TrackingFlags>
void* ezStackAllocator<TrackingFlags>::Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc)
{
  EZ_LOCK(m_Mutex);

  void* ptr = ezAllocator<ezMemoryPolicies::ezStackAllocation, TrackingFlags>::Allocate(uiSize, uiAlign, destructorFunc);

  if (destructorFunc != nullptr)
  {
    ezUInt32 uiIndex = m_DestructData.GetCount();
    m_PtrToDestructDataIndexTable.Insert(ptr, uiIndex);

    auto& data = m_DestructData.ExpandAndGetRef();
    data.m_Func = destructorFunc;
    data.m_Ptr = ptr;
  }

  return ptr;
}

template <ezUInt32 TrackingFlags>
void ezStackAllocator<TrackingFlags>::Deallocate(void* ptr)
{
  EZ_LOCK(m_Mutex);

  ezUInt32 uiIndex;
  if (m_PtrToDestructDataIndexTable.Remove(ptr, &uiIndex))
  {
    auto& data = m_DestructData[uiIndex];
    data.m_Func = nullptr;
    data.m_Ptr = nullptr;
  }

  ezAllocator<ezMemoryPolicies::ezStackAllocation, TrackingFlags>::Deallocate(ptr);
}

EZ_MSVC_ANALYSIS_WARNING_PUSH

// Disable warning for incorrect operator (compiler complains about the TrackingFlags bitwise and in the case that flags = None)
// even with the added guard of a check that it can't be 0.
EZ_MSVC_ANALYSIS_WARNING_DISABLE(6313) 

template <ezUInt32 TrackingFlags>
void ezStackAllocator<TrackingFlags>::Reset()
{
  EZ_LOCK(m_Mutex);

  for (ezUInt32 i = m_DestructData.GetCount(); i-- > 0;)
  {
    auto& data = m_DestructData[i];
    if (data.m_Func != nullptr)
      data.m_Func(data.m_Ptr);
  }
  m_DestructData.Clear();
  m_PtrToDestructDataIndexTable.Clear();

  this->m_allocator.Reset();
  if (TrackingFlags != ezMemoryTrackingFlags::None && (TrackingFlags & ezMemoryTrackingFlags::EnableTracking))
  {
    ezMemoryTracker::RemoveAllAllocations(this->m_Id);
  }
}

EZ_MSVC_ANALYSIS_WARNING_POP
