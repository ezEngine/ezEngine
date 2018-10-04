#pragma once

namespace ezMath
{
  template <>
  EZ_ALWAYS_INLINE ezInt32 BasicType<ezInt32>::MaxValue()
  {
    return 2147483647;
  }

  template <>
  EZ_ALWAYS_INLINE ezUInt32 BasicType<ezUInt32>::MaxValue()
  {
    return 4294967295;
  }

  constexpr EZ_ALWAYS_INLINE ezInt32 RoundUp(ezInt32 value, ezUInt16 multiple)
  {
    //
    return (value >= 0) ? ((value + multiple - 1) / multiple) * multiple : (value / multiple) * multiple;
  }

  constexpr EZ_ALWAYS_INLINE ezInt32 RoundDown(ezInt32 value, ezUInt16 multiple)
  {
    //
    return (value <= 0) ? ((value - multiple + 1) / multiple) * multiple : (value / multiple) * multiple;
  }

  constexpr EZ_ALWAYS_INLINE ezUInt32 RoundUp(ezUInt32 value, ezUInt16 multiple)
  {
    //
    return (value >= 0) ? ((value + multiple - 1) / multiple) * multiple : (value / multiple) * multiple;
  }

  constexpr EZ_ALWAYS_INLINE ezUInt32 RoundDown(ezUInt32 value, ezUInt16 multiple)
  {
    //
    return (value <= 0) ? ((value - multiple + 1) / multiple) * multiple : (value / multiple) * multiple;
  }

  constexpr EZ_ALWAYS_INLINE bool IsOdd(ezInt32 i)
  {
    //
    return ((i & 1) != 0);
  }

  constexpr EZ_ALWAYS_INLINE bool IsEven(ezInt32 i)
  {
    //
    return ((i & 1) == 0);
  }

  inline ezUInt32 Log2i(ezUInt32 val)
  {
    ezInt32 ret = -1;
    while (val != 0)
    {
      val >>= 1;
      ret++;
    }

    return (ezUInt32)ret;
  }

  constexpr EZ_ALWAYS_INLINE int Pow2(int i)
  {
    //
    return (1 << i);
  }

  inline int Pow(int base, int exp)
  {
    int res = 1;
    while (exp > 0)
    {
      res *= base;
      --exp;
    }

    return res;
  }

} // namespace ezMath
