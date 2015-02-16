
template <typename T>
EZ_FORCE_INLINE ezAtomicInteger<T>::ezAtomicInteger() : m_value(0)
{
}

template <typename T>
EZ_FORCE_INLINE ezAtomicInteger<T>::ezAtomicInteger(T value) : 
  m_value(value)
{
}

template <typename T>
EZ_FORCE_INLINE ezAtomicInteger<T>::ezAtomicInteger(const ezAtomicInteger<T>& value) : 
  m_value(value.m_value)
{
}

template <typename T>
EZ_FORCE_INLINE ezAtomicInteger<T>& ezAtomicInteger<T>::operator=(const T value)
{
  m_value = value;
  return *this;
}

template <typename T>
EZ_FORCE_INLINE ezAtomicInteger<T>& ezAtomicInteger<T>::operator=(const ezAtomicInteger<T>& value)
{
  m_value = value.m_value;
  return *this;
}

template <typename T>
EZ_FORCE_INLINE T ezAtomicInteger<T>::Increment()
{
  return ezAtomicUtils::Increment(m_value);
}

template <typename T>
EZ_FORCE_INLINE T ezAtomicInteger<T>::Decrement()
{
  return ezAtomicUtils::Decrement(m_value);
}

template <typename T>
EZ_FORCE_INLINE void ezAtomicInteger<T>::Add(T x)
{
  ezAtomicUtils::Add(m_value, x);
}

template <typename T>
EZ_FORCE_INLINE void ezAtomicInteger<T>::Subtract(T x)
{
  ezAtomicUtils::Add(m_value, -x);
}

template <typename T>
EZ_FORCE_INLINE void ezAtomicInteger<T>::And(T x)
{
  ezAtomicUtils::And(m_value, x);
}

template <typename T>
EZ_FORCE_INLINE void ezAtomicInteger<T>::Or(T x)
{
  ezAtomicUtils::Or(m_value, x);
}

template <typename T>
EZ_FORCE_INLINE void ezAtomicInteger<T>::Xor(T x)
{
  ezAtomicUtils::Xor(m_value, x);
}

template <typename T>
EZ_FORCE_INLINE void ezAtomicInteger<T>::Min(T x)
{
  ezAtomicUtils::Min(m_value, x);
}

template <typename T>
EZ_FORCE_INLINE void ezAtomicInteger<T>::Max(T x)
{
  ezAtomicUtils::Max(m_value, x);
}

template <typename T>
EZ_FORCE_INLINE T ezAtomicInteger<T>::Set(T x)
{
  return ezAtomicUtils::Set(m_value, x);
}

template <typename T>
EZ_FORCE_INLINE bool ezAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return ezAtomicUtils::TestAndSet(m_value, expected, x);
}

template <typename T>
EZ_FORCE_INLINE ezAtomicInteger<T>::operator T() const
{
  return ezAtomicUtils::Read(m_value);
}

