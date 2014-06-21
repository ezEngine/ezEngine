
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
}

