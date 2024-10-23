#include <intrin.h>

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Read(const ezInt32& iSrc)
{
  return _InterlockedOr((long*)(&iSrc), 0);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Read(const ezInt64& iSrc)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = iSrc;
  } while (_InterlockedCompareExchange64(const_cast<ezInt64*>(&iSrc), old, old) != old);
  return old;
#else
  return _InterlockedOr64(const_cast<ezInt64*>(&iSrc), 0);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Increment(ezInt32& ref_iDest)
{
  return _InterlockedIncrement(reinterpret_cast<long*>(&ref_iDest));
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Increment(ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old + 1;
#else
  return _InterlockedIncrement64(&ref_iDest);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Decrement(ezInt32& ref_iDest)
{
  return _InterlockedDecrement(reinterpret_cast<long*>(&ref_iDest));
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Decrement(ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old - 1;
#else
  return _InterlockedDecrement64(&ref_iDest);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::PostIncrement(ezInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), 1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::PostIncrement(ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, 1);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::PostDecrement(ezInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), -1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::PostDecrement(ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old - 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, -1);
#endif
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Add(ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Add(ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old + value, old) != old);
#else
  _InterlockedExchangeAdd64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE void ezAtomicUtils::And(ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedAnd(reinterpret_cast<long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::And(ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old & value, old) != old);
#else
  _InterlockedAnd64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Or(ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedOr(reinterpret_cast<long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Or(ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old | value, old) != old);
#else
  _InterlockedOr64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedXor(reinterpret_cast<long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, old ^ value, old) != old);
#else
  _InterlockedXor64(&ref_iDest, value);
#endif
}


inline void ezAtomicUtils::Min(ezInt32& ref_iDest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = ref_iDest;
    ezInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void ezAtomicUtils::Min(ezInt64& ref_iDest, ezInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt64 iOldValue = ref_iDest;
    ezInt64 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void ezAtomicUtils::Max(ezInt32& ref_iDest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = ref_iDest;
    ezInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void ezAtomicUtils::Max(ezInt64& ref_iDest, ezInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt64 iOldValue = ref_iDest;
    ezInt64 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange64(&ref_iDest, iNewValue, iOldValue) == iOldValue)
      break;
  }
}


inline ezInt32 ezAtomicUtils::Set(ezInt32& ref_iDest, ezInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Set(ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = ref_iDest;
  } while (_InterlockedCompareExchange64(&ref_iDest, value, old) != old);
  return old;
#else
  return _InterlockedExchange64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(ezInt32& ref_iDest, ezInt32 iExpected, ezInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected) == iExpected;
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(ezInt64& ref_iDest, ezInt64 iExpected, ezInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected) == iExpected;
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(void** pDest, void* pExpected, void* value)
{
  return _InterlockedCompareExchangePointer(pDest, value, pExpected) == pExpected;
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::CompareAndSwap(ezInt32& ref_iDest, ezInt32 iExpected, ezInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<long*>(&ref_iDest), value, iExpected);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::CompareAndSwap(ezInt64& ref_iDest, ezInt64 iExpected, ezInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected);
}
