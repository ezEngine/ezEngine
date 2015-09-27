namespace ezInternal
{
  template <typename AllocationPolicy, ezUInt32 TrackingFlags>
  class ezAllocatorImpl : public ezAllocatorBase
  {
  public:
    ezAllocatorImpl(const char* szName, ezAllocatorBase* pParent);
    ~ezAllocatorImpl();

    // ezAllocatorBase implementation
    virtual void* Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc = nullptr) override;
    virtual void Deallocate(void* ptr) override;
    virtual size_t AllocatedSize(const void* ptr) override;
    virtual Stats GetStats() const override;

    ezAllocatorBase* GetParent() const;

  protected:
    AllocationPolicy m_allocator;

    ezAllocatorId m_Id;
    ezThreadID m_ThreadID;
  };

  template <typename AllocationPolicy, ezUInt32 TrackingFlags, bool HasReallocate>
  class ezAllocatorMixinReallocate : public ezAllocatorImpl < AllocationPolicy, TrackingFlags >
  {
  public:
    ezAllocatorMixinReallocate(const char* szName, ezAllocatorBase* pParent);
  };

  template <typename AllocationPolicy, ezUInt32 TrackingFlags>
  class ezAllocatorMixinReallocate<AllocationPolicy, TrackingFlags, true> : public ezAllocatorImpl < AllocationPolicy, TrackingFlags >
  {
  public:
    ezAllocatorMixinReallocate(const char* szName, ezAllocatorBase* pParent);
    virtual void* Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign) override;
  };
};

template <typename A, ezUInt32 TrackingFlags>
EZ_FORCE_INLINE ezInternal::ezAllocatorImpl<A, TrackingFlags>::ezAllocatorImpl(const char* szName, ezAllocatorBase* pParent /* = nullptr */) : 
  m_allocator(pParent),
  m_ThreadID(ezThreadUtils::GetCurrentThreadID())
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    EZ_CHECK_AT_COMPILETIME_MSG((TrackingFlags & ~ezMemoryTrackingFlags::All) == 0, "Invalid tracking flags");
    const ezUInt32 uiTrackingFlags = TrackingFlags;
    ezBitflags<ezMemoryTrackingFlags> flags = *reinterpret_cast<const ezBitflags<ezMemoryTrackingFlags>*>(&uiTrackingFlags);
    this->m_Id = ezMemoryTracker::RegisterAllocator(szName, flags);
  }
}

template <typename A, ezUInt32 TrackingFlags>
ezInternal::ezAllocatorImpl<A, TrackingFlags>::~ezAllocatorImpl()
{
  EZ_ASSERT_RELEASE(m_ThreadID == ezThreadUtils::GetCurrentThreadID(), "Allocator is deleted from another thread");

  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::DeregisterAllocator(this->m_Id);
  }
}

template <typename A, ezUInt32 TrackingFlags>
void* ezInternal::ezAllocatorImpl<A, TrackingFlags>::Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc)
{
  // zero size allocations always return nullptr without tracking (since deallocate nullptr is ignored)
  if (uiSize == 0)
    return nullptr;

  EZ_ASSERT_DEV(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  void* ptr = m_allocator.Allocate(uiSize, uiAlign);
  EZ_ASSERT_DEBUG(ptr != nullptr, "Could not allocate %d bytes. Out of memory?", uiSize);

  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::AddAllocation(this->m_Id, ptr, uiSize, uiAlign);
  }

  return ptr;
}

template <typename A, ezUInt32 TrackingFlags>
void ezInternal::ezAllocatorImpl<A, TrackingFlags>::Deallocate(void* ptr)
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::RemoveAllocation(this->m_Id, ptr);
  }

  m_allocator.Deallocate(ptr);
}

template <typename A, ezUInt32 TrackingFlags>
size_t ezInternal::ezAllocatorImpl<A, TrackingFlags>::AllocatedSize(const void* ptr)
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    return ezMemoryTracker::GetAllocationInfo(this->m_Id, ptr).m_uiSize;
  }

  return 0;
}

template <typename A, ezUInt32 TrackingFlags>
ezAllocatorBase::Stats ezInternal::ezAllocatorImpl<A, TrackingFlags>::GetStats() const
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    return ezMemoryTracker::GetAllocatorStats(this->m_Id);
  }

  return Stats();
}

template <typename A, ezUInt32 TrackingFlags>
EZ_FORCE_INLINE ezAllocatorBase* ezInternal::ezAllocatorImpl<A, TrackingFlags>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, ezUInt32 TrackingFlags, bool HasReallocate>
ezInternal::ezAllocatorMixinReallocate<A, TrackingFlags, HasReallocate>::ezAllocatorMixinReallocate(const char* szName, ezAllocatorBase* pParent)
  : ezAllocatorImpl<A, TrackingFlags>(szName, pParent)
{

}

template <typename A, ezUInt32 TrackingFlags>
ezInternal::ezAllocatorMixinReallocate<A, TrackingFlags, true>::ezAllocatorMixinReallocate(const char* szName, ezAllocatorBase* pParent)
  : ezAllocatorImpl<A, TrackingFlags>(szName, pParent)
{

}

template <typename A, ezUInt32 TrackingFlags>
void* ezInternal::ezAllocatorMixinReallocate<A, TrackingFlags, true>::Reallocate(void* ptr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  if ((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::RemoveAllocation(this->m_Id, ptr);
  }

  void* pNewMem = this->m_allocator.Reallocate(ptr, uiCurrentSize, uiNewSize, uiAlign);

  if((TrackingFlags & ezMemoryTrackingFlags::EnableTracking) != 0)
  {
    ezMemoryTracker::AddAllocation(this->m_Id, pNewMem, uiNewSize, uiAlign);
  }
  return pNewMem;
}