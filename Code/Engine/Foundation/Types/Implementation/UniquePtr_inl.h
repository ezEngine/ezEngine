
template <typename T>
EZ_FORCE_INLINE ezUniquePtr<T>::ezUniquePtr()
{
  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
template <typename U>
EZ_FORCE_INLINE ezUniquePtr<T>::ezUniquePtr(const ezInternal::NewInstance<U>& instance)
{
  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;
}

template <typename T>
template <typename U>
EZ_FORCE_INLINE ezUniquePtr<T>::ezUniquePtr(U* pInstance, ezAllocatorBase* pAllocator)
{
  m_pInstance = pInstance;
  m_pAllocator = pAllocator;
}

template <typename T>
template <typename U>
EZ_FORCE_INLINE ezUniquePtr<T>::ezUniquePtr(ezUniquePtr<U>&& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
EZ_FORCE_INLINE ezUniquePtr<T>::~ezUniquePtr()
{
  Reset();
}

template <typename T>
template <typename U>
EZ_FORCE_INLINE void ezUniquePtr<T>::operator=(const ezInternal::NewInstance<U>& instance)
{
  Reset();

  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;
}

template <typename T>
template <typename U>
EZ_FORCE_INLINE void ezUniquePtr<T>::operator=(ezUniquePtr<U>&& other)
{
  Reset();

  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
EZ_FORCE_INLINE T* ezUniquePtr<T>::Release()
{
  T* pInstance = m_pInstance;
  
  m_pInstance = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
EZ_FORCE_INLINE T* ezUniquePtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
EZ_FORCE_INLINE void ezUniquePtr<T>::Reset()
{
  EZ_DELETE(m_pAllocator, m_pInstance);

  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
EZ_FORCE_INLINE T& ezUniquePtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
EZ_FORCE_INLINE T* ezUniquePtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
EZ_FORCE_INLINE ezUniquePtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator==(const ezUniquePtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator!=(const ezUniquePtr<T>& rhs) const
{
  return m_pInstance != rhs.m_pInstance;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator<(const ezUniquePtr<T>& rhs) const
{
  return m_pInstance < rhs.m_pInstance;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator<=(const ezUniquePtr<T>& rhs) const
{
  return !(rhs < *this);
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator>(const ezUniquePtr<T>& rhs) const
{
  return rhs < *this;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator>=(const ezUniquePtr<T>& rhs) const
{
  return !(*this < rhs);
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator!=(std::nullptr_t) const
{
  return m_pInstance != nullptr;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator<(std::nullptr_t) const
{
  return m_pInstance < nullptr;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator<=(std::nullptr_t) const
{
  return m_pInstance <= nullptr;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator>(std::nullptr_t) const
{
  return m_pInstance > nullptr;
}

template <typename T>
EZ_FORCE_INLINE bool ezUniquePtr<T>::operator>=(std::nullptr_t) const
{
  return m_pInstance >= nullptr;
}
