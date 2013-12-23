
EZ_FORCE_INLINE ezIAllocator::ezIAllocator(const char* szName) :
  m_szName(szName)
{
}

EZ_FORCE_INLINE ezIAllocator::~ezIAllocator()
{
}

EZ_FORCE_INLINE const char* ezIAllocator::GetName() const
{
  return m_szName;
}

EZ_FORCE_INLINE ezIAllocator* ezIAllocator::GetParent() const
{
  return NULL;
}

namespace ezInternal
{
  template <typename T>
  EZ_FORCE_INLINE T* CreateRawBuffer(ezIAllocator* pAllocator, size_t uiCount)
  {
    return static_cast<T*>(pAllocator->Allocate(sizeof(T) * uiCount, EZ_ALIGNMENT_OF(T)));
  }
  
  EZ_FORCE_INLINE void DeleteRawBuffer(ezIAllocator* pAllocator, void* ptr)
  {
    if (ptr != NULL)
    {
      pAllocator->Deallocate(ptr);
    }
  }
  
  template <typename T>
  EZ_FORCE_INLINE void Delete(ezIAllocator* pAllocator, T* ptr)
  {
    if (ptr != NULL)
    {
      ezMemoryUtils::Destruct(ptr, 1);
      pAllocator->Deallocate(ptr);
    }
  }

  template <typename T>
  ezArrayPtr<T> CreateArray(ezIAllocator* pAllocator, ezUInt32 uiCount)
  {
    T* buffer = CreateRawBuffer<T>(pAllocator, uiCount);
    ezMemoryUtils::Construct(buffer, uiCount);

    return ezArrayPtr<T>(buffer, uiCount);
  }

  template <typename T>
  void DeleteArray(ezIAllocator* pAllocator, const ezArrayPtr<T>& arrayPtr)
  {
    T* buffer = arrayPtr.GetPtr();
    if (buffer != NULL)
    {
      ezMemoryUtils::Destruct(buffer, arrayPtr.GetCount()); 
      pAllocator->Deallocate(buffer);
    }
  }
}

