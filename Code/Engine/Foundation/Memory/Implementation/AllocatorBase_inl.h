
EZ_FORCE_INLINE ezAllocatorBase::ezAllocatorBase()
{
}

EZ_FORCE_INLINE ezAllocatorBase::~ezAllocatorBase()
{
}

namespace ezInternal
{
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
  inline void DeleteArray(ezAllocatorBase* pAllocator, const ezArrayPtr<T>& arrayPtr)
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
    EZ_CHECK_AT_COMPILETIME_MSG(!std::is_trivial<T>::value, "Pod type is treated as class, did you forget EZ_DECLARE_POD_TYPE?")

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
    if (ptr == NULL)
    {
      EZ_ASSERT_DEV(uiCurrentCount == 0, "current count must be 0 if ptr is NULL");

      return CreateRawBuffer<T>(pAllocator, uiNewCount);
    }
    return ExtendRawBuffer(ptr, pAllocator, uiCurrentCount, uiNewCount, ezGetTypeClass<T>());
  }

}