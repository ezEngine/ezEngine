
template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::ezAtomicInteger()
  : m_value(0)
{
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::ezAtomicInteger(T value)
  : m_value(value)
{
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::ezAtomicInteger(const ezAtomicInteger<T>& value)
  : m_value(value.m_value)
{
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>& ezAtomicInteger<T>::operator=(const T value)
{
  m_value = value;
  return *this;
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>& ezAtomicInteger<T>::operator=(const ezAtomicInteger<T>& value)
{
  m_value = value.m_value;
  return *this;
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::Increment()
{
  return ezAtomicUtils::Increment(m_value);
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::Decrement()
{
  return ezAtomicUtils::Decrement(m_value);
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::PostIncrement()
{
  return ezAtomicUtils::PostIncrement(m_value);
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::PostDecrement()
{
  return ezAtomicUtils::PostDecrement(m_value);
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Add(T x)
{
  ezAtomicUtils::Add(m_value, x);
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Subtract(T x)
{
  ezAtomicUtils::Add(m_value, -x);
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::And(T x)
{
  ezAtomicUtils::And(m_value, x);
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Or(T x)
{
  ezAtomicUtils::Or(m_value, x);
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Xor(T x)
{
  ezAtomicUtils::Xor(m_value, x);
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Min(T x)
{
  ezAtomicUtils::Min(m_value, x);
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Max(T x)
{
  ezAtomicUtils::Max(m_value, x);
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::Set(T x)
{
  return ezAtomicUtils::Set(m_value, x);
}

template <typename T>
EZ_ALWAYS_INLINE bool ezAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return ezAtomicUtils::TestAndSet(m_value, expected, x);
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::operator T() const
{
  return ezAtomicUtils::Read(m_value);
}

//////////////////////////////////////////////////////////////////////////

EZ_ALWAYS_INLINE ezAtomicBool::ezAtomicBool() = default;
EZ_ALWAYS_INLINE ezAtomicBool::~ezAtomicBool() = default;

EZ_ALWAYS_INLINE ezAtomicBool::ezAtomicBool(bool value)
{
  Set(value);
}

EZ_ALWAYS_INLINE ezAtomicBool::ezAtomicBool(const ezAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

EZ_ALWAYS_INLINE bool ezAtomicBool::Set(bool value)
{
  return m_AtomicInt.Set(value ? 1 : 0) != 0;
}

EZ_ALWAYS_INLINE void ezAtomicBool::operator=(bool value)
{
  Set(value);
}

EZ_ALWAYS_INLINE void ezAtomicBool::operator=(const ezAtomicBool& rhs)
{
  Set(static_cast<bool>(rhs));
}

EZ_ALWAYS_INLINE ezAtomicBool::operator bool() const
{
  return static_cast<ezInt32>(m_AtomicInt) != 0;
}

EZ_ALWAYS_INLINE bool ezAtomicBool::TestAndSet(bool expected, bool newValue)
{
  return m_AtomicInt.TestAndSet(expected ? 1 : 0, newValue ? 1 : 0) != 0;
}
