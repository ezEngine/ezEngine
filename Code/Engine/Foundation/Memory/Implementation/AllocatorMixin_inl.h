namespace ezInternal
{
  template <typename AllocationPolicy, ezAllocatorTrackingMode TrackingMode>
  class ezAllocatorImpl : public ezAllocator
  {
  public:
    ezAllocatorImpl(ezStringView sName, ezAllocator* pParent);
    ~ezAllocatorImpl();

    // ezAllocator implementation
    virtual void* Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc = nullptr) override;
    virtual void Deallocate(void* pPtr) override;
    virtual size_t AllocatedSize(const void* pPtr) override;
    virtual ezAllocatorId GetId() const override;
    virtual Stats GetStats() const override;

    ezAllocator* GetParent() const;

  protected:
    AllocationPolicy m_allocator;

    ezAllocatorId m_Id;
    ezThreadID m_ThreadID;
  };

  template <typename AllocationPolicy, ezAllocatorTrackingMode TrackingMode, bool HasReallocate>
  class ezAllocatorMixinReallocate : public ezAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    ezAllocatorMixinReallocate(ezStringView sName, ezAllocator* pParent);
  };

  template <typename AllocationPolicy, ezAllocatorTrackingMode TrackingMode>
  class ezAllocatorMixinReallocate<AllocationPolicy, TrackingMode, true> : public ezAllocatorImpl<AllocationPolicy, TrackingMode>
  {
  public:
    ezAllocatorMixinReallocate(ezStringView sName, ezAllocator* pParent);
    virtual void* Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign) override;
  };
}; // namespace ezInternal

template <typename A, ezAllocatorTrackingMode TrackingMode>
EZ_FORCE_INLINE ezInternal::ezAllocatorImpl<A, TrackingMode>::ezAllocatorImpl(ezStringView sName, ezAllocator* pParent /* = nullptr */)
  : m_allocator(pParent)
  , m_ThreadID(ezThreadUtils::GetCurrentThreadID())
{
  if constexpr (TrackingMode >= ezAllocatorTrackingMode::Basics)
  {
    this->m_Id = ezMemoryTracker::RegisterAllocator(sName, TrackingMode, pParent != nullptr ? pParent->GetId() : ezAllocatorId());
  }
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
ezInternal::ezAllocatorImpl<A, TrackingMode>::~ezAllocatorImpl()
{
  if constexpr (TrackingMode >= ezAllocatorTrackingMode::Basics)
  {
    ezMemoryTracker::DeregisterAllocator(this->m_Id);
  }
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
void* ezInternal::ezAllocatorImpl<A, TrackingMode>::Allocate(size_t uiSize, size_t uiAlign, ezMemoryUtils::DestructorFunction destructorFunc)
{
  EZ_IGNORE_UNUSED(destructorFunc);

  // zero size allocations always return nullptr without tracking (since deallocate nullptr is ignored)
  if (uiSize == 0)
    return nullptr;

  EZ_ASSERT_DEBUG(ezMath::IsPowerOf2((ezUInt32)uiAlign), "Alignment must be power of two");

  ezTime fAllocationTime = ezTime::Now();

  void* ptr = m_allocator.Allocate(uiSize, uiAlign);
  EZ_ASSERT_DEV(ptr != nullptr, "Could not allocate {0} bytes. Out of memory?", uiSize);

  if constexpr (TrackingMode >= ezAllocatorTrackingMode::AllocationStats)
  {
    ezMemoryTracker::AddAllocation(this->m_Id, TrackingMode, ptr, uiSize, uiAlign, ezTime::Now() - fAllocationTime);
  }

  return ptr;
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
void ezInternal::ezAllocatorImpl<A, TrackingMode>::Deallocate(void* pPtr)
{
  if constexpr (TrackingMode >= ezAllocatorTrackingMode::AllocationStats)
  {
    ezMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  m_allocator.Deallocate(pPtr);
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
size_t ezInternal::ezAllocatorImpl<A, TrackingMode>::AllocatedSize(const void* pPtr)
{
  if constexpr (TrackingMode >= ezAllocatorTrackingMode::AllocationStats)
  {
    return ezMemoryTracker::GetAllocationInfo(this->m_Id, pPtr).m_uiSize;
  }
  else
  {
    return 0;
  }
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
ezAllocatorId ezInternal::ezAllocatorImpl<A, TrackingMode>::GetId() const
{
  return this->m_Id;
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
ezAllocator::Stats ezInternal::ezAllocatorImpl<A, TrackingMode>::GetStats() const
{
  if constexpr (TrackingMode >= ezAllocatorTrackingMode::Basics)
  {
    return ezMemoryTracker::GetAllocatorStats(this->m_Id);
  }
  else
  {
    return Stats();
  }
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
EZ_ALWAYS_INLINE ezAllocator* ezInternal::ezAllocatorImpl<A, TrackingMode>::GetParent() const
{
  return m_allocator.GetParent();
}

template <typename A, ezAllocatorTrackingMode TrackingMode, bool HasReallocate>
ezInternal::ezAllocatorMixinReallocate<A, TrackingMode, HasReallocate>::ezAllocatorMixinReallocate(ezStringView sName, ezAllocator* pParent)
  : ezAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
ezInternal::ezAllocatorMixinReallocate<A, TrackingMode, true>::ezAllocatorMixinReallocate(ezStringView sName, ezAllocator* pParent)
  : ezAllocatorImpl<A, TrackingMode>(sName, pParent)
{
}

template <typename A, ezAllocatorTrackingMode TrackingMode>
void* ezInternal::ezAllocatorMixinReallocate<A, TrackingMode, true>::Reallocate(void* pPtr, size_t uiCurrentSize, size_t uiNewSize, size_t uiAlign)
{
  if constexpr (TrackingMode >= ezAllocatorTrackingMode::AllocationStats)
  {
    ezMemoryTracker::RemoveAllocation(this->m_Id, pPtr);
  }

  ezTime fAllocationTime = ezTime::Now();

  void* pNewMem = this->m_allocator.Reallocate(pPtr, uiCurrentSize, uiNewSize, uiAlign);

  if constexpr (TrackingMode >= ezAllocatorTrackingMode::AllocationStats)
  {
    ezMemoryTracker::AddAllocation(this->m_Id, TrackingMode, pNewMem, uiNewSize, uiAlign, ezTime::Now() - fAllocationTime);
  }

  return pNewMem;
}
