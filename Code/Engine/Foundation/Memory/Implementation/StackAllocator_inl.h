template <ezUInt32 TrackingFlags>
ezStackAllocator<TrackingFlags>::ezStackAllocator(const char* szName, ezAllocatorBase* pParent)
  : ezAllocator<ezMemoryPolicies::ezStackAllocation, TrackingFlags>(szName, pParent)
{

}

template <ezUInt32 TrackingFlags>
void ezStackAllocator<TrackingFlags>::Reset()
{
  this->m_allocator.Reset();
  if (TrackingFlags & ezMemoryTrackingFlags::EnableTracking)
  {
    ezMemoryTracker::RemoveAllAllocations(this->m_Id);
  }
}
