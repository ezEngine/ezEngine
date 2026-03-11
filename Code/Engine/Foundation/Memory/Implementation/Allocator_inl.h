
EZ_ALWAYS_INLINE ezAllocator::ezAllocator() = default;

EZ_ALWAYS_INLINE ezAllocator::~ezAllocator() = default;


namespace ezMath
{
  // due to #include order issues, we have to forward declare this function here

  EZ_FOUNDATION_DLL ezUInt64 SafeMultiply64(ezUInt64 a, ezUInt64 b, ezUInt64 c, ezUInt64 d);
} // namespace ezMath

namespace ezInternal
{
  template <typename T>
  struct NewInstance
  {
    EZ_ALWAYS_INLINE NewInstance(T* pInstance, ezAllocator* pAllocator)
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

    EZ_ALWAYS_INLINE NewInstance(std::nullptr_t) {}

    template <typename U>
    EZ_ALWAYS_INLINE NewInstance<U> Cast()
    {
      return NewInstance<U>(static_cast<U*>(m_pInstance), m_pAllocator);
    }

    EZ_ALWAYS_INLINE operator T*() { return m_pInstance; }

    EZ_ALWAYS_INLINE T* operator->() { return m_pInstance; }

    T* m_pInstance = nullptr;
    ezAllocator* m_pAllocator = nullptr;
  };

  template <typename T>
  EZ_ALWAYS_INLINE bool operator<(const NewInstance<T>& lhs, T* rhs)
  {
    return lhs.m_pInstance < rhs;
  }

  template <typename T>
  EZ_ALWAYS_INLINE bool operator<(T* lhs, const NewInstance<T>& rhs)
  {
    return lhs < rhs.m_pInstance;
  }

  template <typename T>
  EZ_FORCE_INLINE void Delete(ezAllocator* pAllocator, T* pPtr)
  {
    if (pPtr != nullptr)
    {
      ezMemoryUtils::Destruct(pPtr, 1);
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  EZ_FORCE_INLINE T* CreateRawBuffer(ezAllocator* pAllocator, size_t uiCount)
  {
    ezUInt64 safeAllocationSize = ezMath::SafeMultiply64(uiCount, sizeof(T));
    return static_cast<T*>(pAllocator->Allocate(static_cast<size_t>(safeAllocationSize), alignof(T))); // Down-cast to size_t for 32-bit
  }

  EZ_FORCE_INLINE void DeleteRawBuffer(ezAllocator* pAllocator, void* pPtr)
  {
    if (pPtr != nullptr)
    {
      pAllocator->Deallocate(pPtr);
    }
  }

  template <typename T>
  inline ezArrayPtr<T> CreateArray(ezAllocator* pAllocator, ezUInt32 uiCount)
  {
    T* buffer = CreateRawBuffer<T>(pAllocator, uiCount);
    ezMemoryUtils::Construct<SkipTrivialTypes>(buffer, uiCount);

    return ezArrayPtr<T>(buffer, uiCount);
  }

  template <typename T>
  inline void DeleteArray(ezAllocator* pAllocator, ezArrayPtr<T> arrayPtr)
  {
    T* buffer = arrayPtr.GetPtr();
    if (buffer != nullptr)
    {
      ezMemoryUtils::Destruct(buffer, arrayPtr.GetCount());
      pAllocator->Deallocate(buffer);
    }
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, ezAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, ezTypeIsPod)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), alignof(T));
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, ezAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, ezTypeIsMemRelocatable)
  {
    return (T*)pAllocator->Reallocate(pPtr, uiCurrentCount * sizeof(T), uiNewCount * sizeof(T), alignof(T));
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, ezAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount, ezTypeIsClass)
  {
    static_assert(!std::is_trivial<T>::value,
      "POD type is treated as class. Use EZ_DECLARE_POD_TYPE(YourClass) or EZ_DEFINE_AS_POD_TYPE(ExternalClass) to mark it as POD.");

    T* pNewMem = CreateRawBuffer<T>(pAllocator, uiNewCount);
    ezMemoryUtils::RelocateConstruct(pNewMem, pPtr, uiCurrentCount);
    DeleteRawBuffer(pAllocator, pPtr);
    return pNewMem;
  }

  template <typename T>
  EZ_FORCE_INLINE T* ExtendRawBuffer(T* pPtr, ezAllocator* pAllocator, size_t uiCurrentCount, size_t uiNewCount)
  {
    EZ_ASSERT_DEV(uiCurrentCount < uiNewCount, "Shrinking of a buffer is not implemented yet");
    EZ_ASSERT_DEV(!(uiCurrentCount == uiNewCount), "Same size passed in twice.");
    if (pPtr == nullptr)
    {
      EZ_ASSERT_DEV(uiCurrentCount == 0, "current count must be 0 if ptr is nullptr");

      return CreateRawBuffer<T>(pAllocator, uiNewCount);
    }
    return ExtendRawBuffer(pPtr, pAllocator, uiCurrentCount, uiNewCount, ezGetTypeClass<T>());
  }
} // namespace ezInternal
