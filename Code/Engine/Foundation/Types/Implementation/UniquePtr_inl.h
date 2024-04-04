
template <typename T>
EZ_ALWAYS_INLINE ezUniquePtr<T>::ezUniquePtr() = default;

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezUniquePtr<T>::ezUniquePtr(const ezInternal::NewInstance<U>& instance)
{
  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezUniquePtr<T>::ezUniquePtr(U* pInstance, ezAllocator* pAllocator)
{
  m_pInstance = pInstance;
  m_pAllocator = pAllocator;
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezUniquePtr<T>::ezUniquePtr(ezUniquePtr<U>&& other)
{
  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE ezUniquePtr<T>::ezUniquePtr(std::nullptr_t)
{
}

template <typename T>
EZ_ALWAYS_INLINE ezUniquePtr<T>::~ezUniquePtr()
{
  Clear();
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezUniquePtr<T>& ezUniquePtr<T>::operator=(const ezInternal::NewInstance<U>& instance)
{
  Clear();

  m_pInstance = instance.m_pInstance;
  m_pAllocator = instance.m_pAllocator;

  return *this;
}

template <typename T>
template <typename U>
EZ_ALWAYS_INLINE ezUniquePtr<T>& ezUniquePtr<T>::operator=(ezUniquePtr<U>&& other)
{
  Clear();

  m_pInstance = other.m_pInstance;
  m_pAllocator = other.m_pAllocator;

  other.m_pInstance = nullptr;
  other.m_pAllocator = nullptr;

  return *this;
}

template <typename T>
EZ_ALWAYS_INLINE ezUniquePtr<T>& ezUniquePtr<T>::operator=(std::nullptr_t)
{
  Clear();

  return *this;
}

template <typename T>
EZ_ALWAYS_INLINE T* ezUniquePtr<T>::Release()
{
  T* pInstance = m_pInstance;

  m_pInstance = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE T* ezUniquePtr<T>::Release(ezAllocator*& out_pAllocator)
{
  T* pInstance = m_pInstance;
  out_pAllocator = m_pAllocator;

  m_pInstance = nullptr;
  m_pAllocator = nullptr;

  return pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE T* ezUniquePtr<T>::Borrow() const
{
  return m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE void ezUniquePtr<T>::Clear()
{
  if (m_pAllocator != nullptr)
  {
    EZ_DELETE(m_pAllocator, m_pInstance);
  }

  m_pInstance = nullptr;
  m_pAllocator = nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE T& ezUniquePtr<T>::operator*() const
{
  return *m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE T* ezUniquePtr<T>::operator->() const
{
  return m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE ezUniquePtr<T>::operator bool() const
{
  return m_pInstance != nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator==(const ezUniquePtr<T>& rhs) const
{
  return m_pInstance == rhs.m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator!=(const ezUniquePtr<T>& rhs) const
{
  return m_pInstance != rhs.m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator<(const ezUniquePtr<T>& rhs) const
{
  return m_pInstance < rhs.m_pInstance;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator<=(const ezUniquePtr<T>& rhs) const
{
  return !(rhs < *this);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator>(const ezUniquePtr<T>& rhs) const
{
  return rhs < *this;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator>=(const ezUniquePtr<T>& rhs) const
{
  return !(*this < rhs);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator==(std::nullptr_t) const
{
  return m_pInstance == nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator!=(std::nullptr_t) const
{
  return m_pInstance != nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator<(std::nullptr_t) const
{
  return m_pInstance < nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator<=(std::nullptr_t) const
{
  return m_pInstance <= nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator>(std::nullptr_t) const
{
  return m_pInstance > nullptr;
}

template <typename T>
EZ_ALWAYS_INLINE bool ezUniquePtr<T>::operator>=(std::nullptr_t) const
{
  return m_pInstance >= nullptr;
}

//////////////////////////////////////////////////////////////////////////
// free functions

template <typename T>
EZ_ALWAYS_INLINE bool operator==(const ezUniquePtr<T>& lhs, const T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
EZ_ALWAYS_INLINE bool operator==(const ezUniquePtr<T>& lhs, T* rhs)
{
  return lhs.Borrow() == rhs;
}

template <typename T>
EZ_ALWAYS_INLINE bool operator!=(const ezUniquePtr<T>& lhs, const T* rhs)
{
  return lhs.Borrow() != rhs;
}

template <typename T>
EZ_ALWAYS_INLINE bool operator!=(const ezUniquePtr<T>& lhs, T* rhs)
{
  return lhs.Borrow() != rhs;
}

template <typename T>
EZ_ALWAYS_INLINE bool operator==(const T* lhs, const ezUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}

template <typename T>
EZ_ALWAYS_INLINE bool operator==(T* lhs, const ezUniquePtr<T>& rhs)
{
  return lhs == rhs.Borrow();
}

template <typename T>
EZ_ALWAYS_INLINE bool operator!=(const T* lhs, const ezUniquePtr<T>& rhs)
{
  return lhs != rhs.Borrow();
}

template <typename T>
EZ_ALWAYS_INLINE bool operator!=(T* lhs, const ezUniquePtr<T>& rhs)
{
  return lhs != rhs.Borrow();
}
