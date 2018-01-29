
template <typename T>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr()
{
  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr(const ezInternal::NewInstance<U>& instance)
{
  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr(U* pInstance, ezAllocatorBase* pAllocator)
{
  m_pInstance = pInstance;
  m_pAllocator = pAllocator;

  AddReferenceIfValid();
}

template <typename T>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr(const ezSharedPtr<T>& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr(const ezSharedPtr<U>& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr(ezSharedPtr<U>&& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr(ezUniquePtr<U>&& other)
{
  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();
}

template <typename T>
EZ_ALWAYS_INLINE ezSharedPtr<T>::ezSharedPtr(std::nullptr_t)
{
  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE ezSharedPtr<T>::~ezSharedPtr()
{
  ReleaseReferenceIfValid();
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::operator=(const ezInternal::NewInstance<U>& instance)
{
  ReleaseReferenceIfValid();

  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  AddReferenceIfValid();
}

template <typename T>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::operator=(const ezSharedPtr<T>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::operator=(const ezSharedPtr<U>& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    AddReferenceIfValid();
  }
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::operator=(ezSharedPtr<U>&& other)
{
  if (m_pInstance != other.m_pInstance)
  {
    ReleaseReferenceIfValid();

    m_pInstance = other.m_pInstance;
    m_pAllocator = other.m_pAllocator;

    other.m_pInstance = nullptr;
    other.m_pAllocator = nullptr;
  }
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::operator=(ezUniquePtr<U>&& other)
{
  ReleaseReferenceIfValid();

  m_pInstance = other.Release(m_pAllocator);

  AddReferenceIfValid();
}

template <typename T>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::operator=(std::nullptr_t)
{
  ReleaseReferenceIfValid();
}

template <typename T>
EZ_ALWAYS_INLINE T* ezSharedPtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::Reset()
{
  ReleaseReferenceIfValid();
}

template <typename T>
EZ_ALWAYS_INLINE T& ezSharedPtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE T* ezSharedPtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE ezSharedPtr<T>::operator const T*() const
{
  return m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE ezSharedPtr<T>::operator T*()
{
  return m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE ezSharedPtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator==(const ezSharedPtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator!=(const ezSharedPtr<T>& rhs) const
{
  return m_pInstance != rhs.m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator<(const ezSharedPtr<T>& rhs) const
{
  return m_pInstance < rhs.m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator<=(const ezSharedPtr<T>& rhs) const
{
  return !(rhs < *this);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator>(const ezSharedPtr<T>& rhs) const
{
  return rhs < *this;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator>=(const ezSharedPtr<T>& rhs) const
{
  return !(*this < rhs);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator!=(std::nullptr_t) const
{
  return m_pInstance != nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator<(std::nullptr_t) const
{
  return m_pInstance < nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator<=(std::nullptr_t) const
{
  return m_pInstance <= nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator>(std::nullptr_t) const
{
  return m_pInstance > nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezSharedPtr<T>::operator>=(std::nullptr_t) const
{
  return m_pInstance >= nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::AddReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    m_pInstance->AddRef();
  }
}

template <typename T>
EZ_ALWAYS_INLINE void ezSharedPtr<T>::ReleaseReferenceIfValid()
{
  if (m_pInstance != nullptr)
  {
    if (m_pInstance->ReleaseRef() == 0)
    {
      auto pNonConstInstance = const_cast<ezTypeTraits<T>::NonConstType*>(m_pInstance);
      EZ_DELETE(m_pAllocator, pNonConstInstance);
    }

    m_pInstance = nullptr;
    m_pAllocator = nullptr;
  }
}

