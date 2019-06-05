
EZ_ALWAYS_INLINE ezAllocatorBase::ezAllocatorBase() {}

EZ_ALWAYS_INLINE ezAllocatorBase::~ezAllocatorBase() {}

namespace ezInternal
{
  template <typename T>
  struct NewInstance
  {
    EZ_ALWAYS_INLINE NewInstance(T* pInstance, ezAllocatorBase* pAllocator)
    {
      m_pInstance = pInstance;
      m_pAllocator = pAllocator;
    }

    template <typename U>
    EZ_ALWAYS_INLINE NewInstance(NewInstance<U>&& other)
    {
      m_pInstance = other.m_pInstance;
      m_pAllocator = other.m_pAllocator;

      other.m_pInstance = nullptr;
      other.m_pAllocator = nullptr;
    }

    template <typename U>
    EZ_ALWAYS_INLINE NewInstance<U> Cast()
    {
      return NewInstance<U>(static_cast<U*>(m_pInstance), m_pAllocator);
    }

    EZ_ALWAYS_INLINE operator T*() { return m_pInstance; }

    EZ_ALWAYS_INLINE T* operator->() { return m_pInstance; }

    T* m_pInstance;
    ezAllocatorBase* m_pAllocator;
  };


  template <typename T>
  EZ_FORCE_INLINE void Delete(ezAllocatorBase* pAllocator, T* ptr)
  {
    if (ptr != nullptr)
    {
      ezMemoryUtils::Destruct(ptr, 1);
      pAllocator->Deallocate(ptr);
    }
  }

  template <typename T>
  EZ_FORCE_INLINE T* CreateRawBuffer(ezAllocatorBase* pAllocator, size_t uiCount)
  {
    return static_cast<T*>(pAllocator->Allocate(sizeof(T) * uiCount, EZ_ALIGNMENT_OF(T)));
  }

  EZ_FORCE_INLINE void DeleteRawBuffer(ezAllocatorBase* pAllocator, void* ptr)
  {
    if (ptr != nullptr)
    {
      pAllocator->Deallocate(ptr);
    }
  }

  template <typename T>
  inline ezArrayPtr<T> CreateArray(ezAllocatorBase* pAllocator, ezUInt32 uiCount)
  {
    T* buffer = CreateRawBuffer<T>(pAllocator, uiCount);
    ezMemoryUtils::Construct(buffer, uiCount);

    return ezArrayPtr<T>(buffer, uiCount);
  }

  template <typename T>
  inline void DeleteArray(ezAllocatorBase* pAllocator, ezArrayPtr<T> arrayPtr)
  {
    T* buffer = arrayPtr.GetPtr();
    if (buffer != nullptr)
    {
      ezMemoryUtils::Destruct(buffer, arrayPtr.GetCount());
      pAllocator->Deallocate(buffer);
    }
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* ptr, ezAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount, ezTypeIsPod)
  {
    return (T*)pAllocator->Reallocate(ptr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), EZ_ALIGNMENT_OF(T));
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* ptr, ezAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount, ezTypeIsMemRelocatable)
  {
    return (T*)pAllocator->Reallocate(ptr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), EZ_ALIGNMENT_OF(T));
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* ptr, ezAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount, ezTypeIsClass)
  {
    EZ_CHECK_AT_COMPILETIME_MSG(!std::is_trivial<T>::value,
      "POD type is treated as class. Use EZ_DECLARE_POD_TYPE(YourClass) or EZ_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.");

    T* pNewMem = CreateRawBuffer<T>(pAllocator, uiNewCount);
    ezMemoryUtils::RelocateConstruct(pNewMem, ptr, uiCurrentCount);
    DeleteRawBuffer(pAllocator, ptr);
    return pNewMem;
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* ptr, ezAllocatorBase* pAllocator, size_t uiCurrentCount, size_t uiNewCount)
  {
    EZ_ASSERT_DEV(uiCurrentCount < uiNewCount, "Shrinking of a buffer is not implemented yet");
    EZ_ASSERT_DEV(!(uiCurrentCount == uiNewCount), "Same size passed in twice.");
    if (ptr == nullptr)
    {
      EZ_ASSERT_DEV(uiCurrentCount == 0, "current count must be 0 if ptr is nullptr");

      return CreateRawBuffer<T>(pAllocator, uiNewCount);
    }
    return ExtendRawBuffer(ptr, pAllocator, uiCurrentCount, uiNewCount, ezGetTypeClass<T>());
  }
} // namespace ezInternal
