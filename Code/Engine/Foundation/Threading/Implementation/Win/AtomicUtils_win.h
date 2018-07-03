#ifdef EZ_ATOMICUTLS_WIN_INL_H_INCLUDED
#error "This file must not be included twice."
#endif

#define EZ_ATOMICUTLS_WIN_INL_H_INCLUDED

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Read(volatile const ezInt32& src)
{
  return _InterlockedOr((volatile LONG*)(&src), 0);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Read(volatile const ezInt64& src)
{
  return InterlockedOr64(const_cast<volatile ezInt64*>(&src), 0);
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Increment(volatile ezInt32& dest)
{
  return InterlockedIncrement(reinterpret_cast<volatile LONG*>(&dest));
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Increment(volatile ezInt64& dest)
{
  return InterlockedIncrement64(&dest);
}


EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Decrement(volatile ezInt32& dest)
{
  return InterlockedDecrement(reinterpret_cast<volatile LONG*>(&dest));
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Decrement(volatile ezInt64& dest)
{
  return InterlockedDecrement64(&dest);
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Add(volatile ezInt32& dest, ezInt32 value)
{
  InterlockedExchangeAdd(reinterpret_cast<volatile LONG*>(&dest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Add(volatile ezInt64& dest, ezInt64 value)
{
  InterlockedExchangeAdd64(&dest, value);
}


EZ_ALWAYS_INLINE void ezAtomicUtils::And(volatile ezInt32& dest, ezInt32 value)
{
  _InterlockedAnd(reinterpret_cast<volatile LONG*>(&dest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::And(volatile ezInt64& dest, ezInt64 value)
{
  InterlockedAnd64(&dest, value);
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Or(volatile ezInt32& dest, ezInt32 value)
{
  _InterlockedOr(reinterpret_cast<volatile LONG*>(&dest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Or(volatile ezInt64& dest, ezInt64 value)
{
  InterlockedOr64(&dest, value);
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(volatile ezInt32& dest, ezInt32 value)
{
  _InterlockedXor(reinterpret_cast<volatile LONG*>(&dest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(volatile ezInt64& dest, ezInt64 value)
{
  InterlockedXor64(&dest, value);
}


inline void ezAtomicUtils::Min(volatile ezInt32& dest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = dest;
    ezInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&dest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void ezAtomicUtils::Min(volatile ezInt64& dest, ezInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt64 iOldValue = dest;
    ezInt64 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (InterlockedCompareExchange64(&dest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}


inline void ezAtomicUtils::Max(volatile ezInt32& dest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = dest;
    ezInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&dest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void ezAtomicUtils::Max(volatile ezInt64& dest, ezInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt64 iOldValue = dest;
    ezInt64 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (InterlockedCompareExchange64(&dest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}


inline ezInt32 ezAtomicUtils::Set(volatile ezInt32& dest, ezInt32 value)
{
  return InterlockedExchange(reinterpret_cast<volatile LONG*>(&dest), value);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Set(volatile ezInt64& dest, ezInt64 value)
{
  return InterlockedExchange64(&dest, value);
}


EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(volatile ezInt32& dest, ezInt32 expected, ezInt32 value)
{
  return InterlockedCompareExchange(reinterpret_cast<volatile LONG*>(&dest), value, expected) == expected;
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(volatile ezInt64& dest, ezInt64 expected, ezInt64 value)
{
  return InterlockedCompareExchange64(&dest, value, expected) == expected;
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(void** volatile dest, void* expected, void* value)
{
  return InterlockedCompareExchangePointer(dest, value, expected) == expected;
}
