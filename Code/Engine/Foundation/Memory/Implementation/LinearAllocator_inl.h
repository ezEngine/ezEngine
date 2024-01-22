template <ezAllocatorTrackingMode TrackingMode>
ezLinearAllocator<TrackingMode>::ezLinearAllocator(ezStringView sName, ezAllocator* pParent)
  : ezAllocatorWithPolicy<ezMemoryPolicies::ezAllocPolicyStack, TrackingMode>(sName, pParent)
  , m_DestructData(pParent)
  , m_PtrToDestructDataIndexTable(pParent)
{
}

template <ezAllocatorTrackingMode TrackingMode>
ezLinearAllocator<TrackingMode>::~ezLinearAllocator()
{
  Reset();
}

template <ezAllocatorTrackingMode TrackingMode>
void* ezLinearAllocator<TrackingMode>::Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc)
{
  EZ_LOCK(m_Mutex);

  void* ptr = ezAllocatorWithPolicy<ezMemoryPolicies::ezAllocPolicyStack, TrackingMode>::Allocate(uiSize, uiAlign, destructorFunc);

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

template <ezAllocatorTrackingMode TrackingMode>
void ezLinearAllocator<TrackingMode>::Deallocate(void* pPtr)
{
  EZ_LOCK(m_Mutex);

  ezUInt32 uiIndex;
  if (m_PtrToDestructDataIndexTable.Remove(pPtr, &uiIndex))
  {
    auto& data = m_DestructData[uiIndex];
    data.m_Func = nullptr;
    data.m_Ptr = nullptr;
  }

  ezAllocatorWithPolicy<ezMemoryPolicies::ezAllocPolicyStack, TrackingMode>::Deallocate(pPtr);
}

EZ_MSVC_ANALYSIS_WARNING_PUSH

// Disable warning for incorrect operator (compiler complains about the TrackingMode bitwise and in the case that flags = None)
// even with the added guard of a check that it can't be 0.
EZ_MSVC_ANALYSIS_WARNING_DISABLE(6313)

template <ezAllocatorTrackingMode TrackingMode>
void ezLinearAllocator<TrackingMode>::Reset()
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
  if constexpr (TrackingMode >= ezAllocatorTrackingMode::AllocationStats)
  {
    ezMemoryTracker::RemoveAllAllocations(this->m_Id);
  }
  else if constexpr (TrackingMode >= ezAllocatorTrackingMode::Basics)
  {
    ezAllocator::Stats stats;
    this->m_allocator.FillStats(stats);

    ezMemoryTracker::SetAllocatorStats(this->m_Id, stats);
  }
}
EZ_MSVC_ANALYSIS_WARNING_POP
