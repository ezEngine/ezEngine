#ifdef EZ_ATOMICUTLS_WIN_INL_H_INCLUDED
#  error "This file must not be included twice."
#endif

#define EZ_ATOMICUTLS_WIN_INL_H_INCLUDED

#include <intrin.h>

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Read(volatile const ezInt32& iSrc)
{
  return _InterlockedOr((volatile long*)(&iSrc), 0);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Read(volatile const ezInt64& iSrc)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = src;
  } while (_InterlockedCompareExchange64(const_cast<volatile ezInt64*>(&src), old, old) != old);
  return old;
#else
  return _InterlockedOr64(const_cast<volatile ezInt64*>(&iSrc), 0);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Increment(volatile ezInt32& ref_iDest)
{
  return _InterlockedIncrement(reinterpret_cast<volatile long*>(&ref_iDest));
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Increment(volatile ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + 1, old) != old);
  return old + 1;
#else
  return _InterlockedIncrement64(&ref_iDest);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Decrement(volatile ezInt32& ref_iDest)
{
  return _InterlockedDecrement(reinterpret_cast<volatile long*>(&ref_iDest));
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Decrement(volatile ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old - 1, old) != old);
  return old - 1;
#else
  return _InterlockedDecrement64(&ref_iDest);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::PostIncrement(volatile ezInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), 1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::PostIncrement(volatile ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, 1);
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::PostDecrement(volatile ezInt32& ref_iDest)
{
  return _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), -1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::PostDecrement(volatile ezInt64& ref_iDest)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old - 1, old) != old);
  return old;
#else
  return _InterlockedExchangeAdd64(&ref_iDest, -1);
#endif
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Add(volatile ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedExchangeAdd(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Add(volatile ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old + value, old) != old);
#else
  _InterlockedExchangeAdd64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE void ezAtomicUtils::And(volatile ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedAnd(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::And(volatile ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old & value, old) != old);
#else
  _InterlockedAnd64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Or(volatile ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedOr(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Or(volatile ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old | value, old) != old);
#else
  _InterlockedOr64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(volatile ezInt32& ref_iDest, ezInt32 value)
{
  _InterlockedXor(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(volatile ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, old ^ value, old) != old);
#else
  _InterlockedXor64(&ref_iDest, value);
#endif
}


inline void ezAtomicUtils::Min(volatile ezInt32& ref_iDest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = ref_iDest;
    ezInt32 iNewValue = value < iOldValue ? value : iOldValue; // do Min manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void ezAtomicUtils::Min(volatile ezInt64& ref_iDest, ezInt64 value)
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

inline void ezAtomicUtils::Max(volatile ezInt32& ref_iDest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = ref_iDest;
    ezInt32 iNewValue = iOldValue < value ? value : iOldValue; // do Max manually here, to break #include cycles

    if (_InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), iNewValue, iOldValue) == iOldValue)
      break;
  }
}

inline void ezAtomicUtils::Max(volatile ezInt64& ref_iDest, ezInt64 value)
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


inline ezInt32 ezAtomicUtils::Set(volatile ezInt32& ref_iDest, ezInt32 value)
{
  return _InterlockedExchange(reinterpret_cast<volatile long*>(&ref_iDest), value);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Set(volatile ezInt64& ref_iDest, ezInt64 value)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  ezInt64 old;
  do
  {
    old = dest;
  } while (_InterlockedCompareExchange64(&dest, value, old) != old);
  return old;
#else
  return _InterlockedExchange64(&ref_iDest, value);
#endif
}


EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(volatile ezInt32& ref_iDest, ezInt32 iExpected, ezInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), value, iExpected) == iExpected;
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(volatile ezInt64& ref_iDest, ezInt64 iExpected, ezInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected) == iExpected;
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(void** volatile pDest, void* pExpected, void* value)
{
  return _InterlockedCompareExchangePointer(pDest, value, pExpected) == pExpected;
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::CompareAndSwap(volatile ezInt32& ref_iDest, ezInt32 iExpected, ezInt32 value)
{
  return _InterlockedCompareExchange(reinterpret_cast<volatile long*>(&ref_iDest), value, iExpected);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::CompareAndSwap(volatile ezInt64& ref_iDest, ezInt64 iExpected, ezInt64 value)
{
  return _InterlockedCompareExchange64(&ref_iDest, value, iExpected);
}
