
template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::ezAtomicInteger()
  : m_Value(0)
{
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::ezAtomicInteger(T value)
  : m_Value(static_cast<UnderlyingType>(value))
{
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::ezAtomicInteger(const ezAtomicInteger<T>& value)
  : m_Value(ezAtomicUtils::Read(value.m_Value))
{
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>& ezAtomicInteger<T>::operator=(const T value)
{
  Set(value);
  return *this;
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>& ezAtomicInteger<T>::operator=(const ezAtomicInteger<T>& value)
{
  Set(ezAtomicUtils::Read(value.m_Value));
  return *this;
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::Increment()
{
  return static_cast<T>(ezAtomicUtils::Increment(m_Value));
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::Decrement()
{
  return static_cast<T>(ezAtomicUtils::Decrement(m_Value));
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::PostIncrement()
{
  return static_cast<T>(ezAtomicUtils::PostIncrement(m_Value));
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::PostDecrement()
{
  return static_cast<T>(ezAtomicUtils::PostDecrement(m_Value));
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Add(T x)
{
  ezAtomicUtils::Add(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Subtract(T x)
{
  ezAtomicUtils::Add(m_Value, -static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::And(T x)
{
  ezAtomicUtils::And(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Or(T x)
{
  ezAtomicUtils::Or(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Xor(T x)
{
  ezAtomicUtils::Xor(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Min(T x)
{
  ezAtomicUtils::Min(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE void ezAtomicInteger<T>::Max(T x)
{
  ezAtomicUtils::Max(m_Value, static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::Set(T x)
{
  return static_cast<T>(ezAtomicUtils::Set(m_Value, static_cast<UnderlyingType>(x)));
}

template <typename T>
EZ_ALWAYS_INLINE bool ezAtomicInteger<T>::TestAndSet(T expected, T x)
{
  return ezAtomicUtils::TestAndSet(m_Value, static_cast<UnderlyingType>(expected), static_cast<UnderlyingType>(x));
}

template <typename T>
EZ_ALWAYS_INLINE T ezAtomicInteger<T>::CompareAndSwap(T expected, T x)
{
  return static_cast<T>(ezAtomicUtils::CompareAndSwap(m_Value, static_cast<UnderlyingType>(expected), static_cast<UnderlyingType>(x)));
}

template <typename T>
EZ_ALWAYS_INLINE ezAtomicInteger<T>::operator T() const
{
  return static_cast<T>(ezAtomicUtils::Read(m_Value));
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
  return m_iAtomicInt.Set(value ? 1 : 0) != 0;
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
  return static_cast<ezInt32>(m_iAtomicInt) != 0;
}

EZ_ALWAYS_INLINE bool ezAtomicBool::TestAndSet(bool bExpected, bool bNewValue)
{
  return m_iAtomicInt.TestAndSet(bExpected ? 1 : 0, bNewValue ? 1 : 0) != 0;
}
