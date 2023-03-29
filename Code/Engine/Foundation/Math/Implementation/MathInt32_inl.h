#pragma once

namespace ezMath
{
  constexpr EZ_ALWAYS_INLINE ezInt32 RoundUp(ezInt32 value, ezUInt16 uiMultiple)
  {
    //
    return (value >= 0) ? ((value + uiMultiple - 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr EZ_ALWAYS_INLINE ezInt32 RoundDown(ezInt32 value, ezUInt16 uiMultiple)
  {
    //
    return (value <= 0) ? ((value - uiMultiple + 1) / uiMultiple) * uiMultiple : (value / uiMultiple) * uiMultiple;
  }

  constexpr EZ_ALWAYS_INLINE ezUInt32 RoundUp(ezUInt32 value, ezUInt16 uiMultiple)
  {
    //
    return ((value + uiMultiple - 1) / uiMultiple) * uiMultiple;
  }

  constexpr EZ_ALWAYS_INLINE ezUInt32 RoundDown(ezUInt32 value, ezUInt16 uiMultiple)
  {
    //
    return (value / uiMultiple) * uiMultiple;
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

  EZ_ALWAYS_INLINE ezUInt32 Log2i(ezUInt32 uiVal)
  {
    return (uiVal != 0) ? FirstBitHigh(uiVal) : -1;
  }

  constexpr EZ_ALWAYS_INLINE int Pow2(int i)
  {
    //
    return (1 << i);
  }

  inline int Pow(int iBase, int iExp)
  {
    int res = 1;
    while (iExp > 0)
    {
      res *= iBase;
      --iExp;
    }

    return res;
  }

} // namespace ezMath
