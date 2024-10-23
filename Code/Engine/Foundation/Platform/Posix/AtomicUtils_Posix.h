#include <Foundation/Math/Math.h>

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Read(const ezInt32& src)
{
  return __sync_fetch_and_or(const_cast<ezInt32*>(&src), 0);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Read(const ezInt64& src)
{
  return __sync_fetch_and_or_8(const_cast<ezInt64*>(&src), 0);
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Increment(ezInt32& dest)
{
  return __sync_add_and_fetch(&dest, 1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Increment(ezInt64& dest)
{
  return __sync_add_and_fetch_8(&dest, 1);
}


EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Decrement(ezInt32& dest)
{
  return __sync_sub_and_fetch(&dest, 1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Decrement(ezInt64& dest)
{
  return __sync_sub_and_fetch_8(&dest, 1);
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::PostIncrement(ezInt32& dest)
{
  return __sync_fetch_and_add(&dest, 1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::PostIncrement(ezInt64& dest)
{
  return __sync_fetch_and_add_8(&dest, 1);
}


EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::PostDecrement(ezInt32& dest)
{
  return __sync_fetch_and_sub(&dest, 1);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::PostDecrement(ezInt64& dest)
{
  return __sync_fetch_and_sub_8(&dest, 1);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Add(ezInt32& dest, ezInt32 value)
{
  __sync_fetch_and_add(&dest, value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Add(ezInt64& dest, ezInt64 value)
{
  __sync_fetch_and_add_8(&dest, value);
}


EZ_ALWAYS_INLINE void ezAtomicUtils::And(ezInt32& dest, ezInt32 value)
{
  __sync_fetch_and_and(&dest, value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::And(ezInt64& dest, ezInt64 value)
{
  __sync_fetch_and_and_8(&dest, value);
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Or(ezInt32& dest, ezInt32 value)
{
  __sync_fetch_and_or(&dest, value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Or(ezInt64& dest, ezInt64 value)
{
  __sync_fetch_and_or_8(&dest, value);
}


EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(ezInt32& dest, ezInt32 value)
{
  __sync_fetch_and_xor(&dest, value);
}

EZ_ALWAYS_INLINE void ezAtomicUtils::Xor(ezInt64& dest, ezInt64 value)
{
  __sync_fetch_and_xor_8(&dest, value);
}


EZ_FORCE_INLINE void ezAtomicUtils::Min(ezInt32& dest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = dest;
    ezInt32 iNewValue = ezMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

EZ_FORCE_INLINE void ezAtomicUtils::Min(ezInt64& dest, ezInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt64 iOldValue = dest;
    ezInt64 iNewValue = ezMath::Min(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


EZ_FORCE_INLINE void ezAtomicUtils::Max(ezInt32& dest, ezInt32 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt32 iOldValue = dest;
    ezInt32 iNewValue = ezMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap(&dest, iOldValue, iNewValue))
      break;
  }
}

EZ_FORCE_INLINE void ezAtomicUtils::Max(ezInt64& dest, ezInt64 value)
{
  // tries to exchange dest with the new value as long as the oldValue is not what we expected
  while (true)
  {
    ezInt64 iOldValue = dest;
    ezInt64 iNewValue = ezMath::Max(iOldValue, value);

    if (__sync_bool_compare_and_swap_8(&dest, iOldValue, iNewValue))
      break;
  }
}


EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::Set(ezInt32& dest, ezInt32 value)
{
  return __sync_lock_test_and_set(&dest, value);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::Set(ezInt64& dest, ezInt64 value)
{
  return __sync_lock_test_and_set_8(&dest, value);
}


EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(ezInt32& dest, ezInt32 expected, ezInt32 value)
{
  return __sync_bool_compare_and_swap(&dest, expected, value);
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(ezInt64& dest, ezInt64 expected, ezInt64 value)
{
  return __sync_bool_compare_and_swap_8(&dest, expected, value);
}

EZ_ALWAYS_INLINE bool ezAtomicUtils::TestAndSet(void** dest, void* expected, void* value)
{
#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  ezUInt64* puiTemp = reinterpret_cast<ezUInt64*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<ezUInt64>(expected), reinterpret_cast<ezUInt64>(value));
#else
  ezUInt32* puiTemp = reinterpret_cast<ezUInt32*>(dest);
  return __sync_bool_compare_and_swap(puiTemp, reinterpret_cast<ezUInt32>(expected), reinterpret_cast<ezUInt32>(value));
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezAtomicUtils::CompareAndSwap(ezInt32& dest, ezInt32 expected, ezInt32 value)
{
  return __sync_val_compare_and_swap(&dest, expected, value);
}

EZ_ALWAYS_INLINE ezInt64 ezAtomicUtils::CompareAndSwap(ezInt64& dest, ezInt64 expected, ezInt64 value)
{
  return __sync_val_compare_and_swap_8(&dest, expected, value);
}
