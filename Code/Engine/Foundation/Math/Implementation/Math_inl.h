#pragma once

#include <algorithm>

namespace ezMath
{
  inline ezUInt32 Log2i(ezUInt32 val) 
  {
    ezInt32 ret = -1;
    while (val != 0) 
    {
      val >>= 1;
      ret++;
    }

    return (ezUInt32) ret;
  }

  EZ_FORCE_INLINE int Pow2(int i)
  {
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

  template <typename T>
  EZ_FORCE_INLINE T Square(T f)
  { 
    return (f * f); 
  }

  template <typename T>
  EZ_FORCE_INLINE T Sign(T f) 
  {
    return (f < 0 ? T (-1) : f > 0 ? T (1) : 0);
  }

  template <typename T>
  EZ_FORCE_INLINE T Abs(T f)
  {
    return (f < 0 ? -f : f);
  }

  template <typename T>
  EZ_FORCE_INLINE T Min(T f1, T f2)
  {
    return (f2 < f1 ? f2 : f1);
  }

  template <typename T>
  EZ_FORCE_INLINE T Min(T f1, T f2, T f3)
  {
    return Min(Min (f1, f2), f3);
  }

  template <typename T>
  EZ_FORCE_INLINE T Min(T f1, T f2, T f3, T f4)
  {
    return Min(Min (f1, f2), Min (f3, f4));
  }

  template <typename T>
  EZ_FORCE_INLINE T Max(T f1, T f2)
  {
    return (f1 > f2 ? f1 : f2);
  }

  template <typename T>
  EZ_FORCE_INLINE T Max(T f1, T f2, T f3)
  {
    return Max(Max (f1, f2), f3);
  }

  template <typename T>
  EZ_FORCE_INLINE T Max(T f1, T f2, T f3, T f4)
  {
    return Max(Max (f1, f2), Max (f3, f4));
  }

  template <typename T>
  EZ_FORCE_INLINE T Clamp(T value, T min_val, T max_val) 
  { 
    if (value < min_val) return (min_val); 
    if (max_val < value) return (max_val);	
    return (value);	
  }

  inline ezInt32 Floor(ezInt32 i, ezUInt32 uiMultiple)
  {
    if (i < 0)
    {
      const ezInt32 iDivides = (i+1) / (ezInt32) uiMultiple;

      return ((iDivides-1) * uiMultiple);
    }

    const ezInt32 iDivides = i / (ezInt32) uiMultiple;
    return (iDivides * uiMultiple);
  }

  inline ezInt32 Ceil(ezInt32 i, ezUInt32 uiMultiple)
  {
    if (i < 0)
    {
      const ezInt32 iDivides = i / (ezInt32) uiMultiple;

      return (iDivides * uiMultiple);
    }

    ezInt32 iDivides = (i-1) / (ezInt32) uiMultiple;
    return ((iDivides+1) * uiMultiple);
  }

  template<typename Type>
  Type Invert(Type f)
  {
    return ((Type) 1) / f;
  }

  EZ_FORCE_INLINE bool IsOdd(ezInt32 i)
  {
    return ((i & 1) != 0);
  }

  EZ_FORCE_INLINE bool IsEven(ezInt32 i)
  {
    return ((i & 1) == 0);
  }

  template <typename T>
  EZ_FORCE_INLINE void Swap(T& f1, T& f2)
  { 
    std::swap(f1, f2);
  }

  template <typename T>
  EZ_FORCE_INLINE T Lerp(T f1, T f2, float factor)
  {
    EZ_ASSERT_DEBUG ((factor >= -0.00001f) && (factor <= 1.0f + 0.00001f), "lerp: factor %.2f is not in the range [0; 1]", factor);

    return (T) (f1 + (factor * (f2 - f1)));
  }

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  EZ_FORCE_INLINE T Step(T value, T edge)
  {
    return (value >= edge ? T (1) : T (0));
  }

  EZ_FORCE_INLINE bool IsPowerOf2(ezInt32 value)
  {
    if (value < 1) 
      return false;

    return ((value & (value - 1)) == 0);
  }

  template<typename Type>
  bool IsEqual(Type lhs, Type rhs, Type fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  template <typename T>
  inline bool IsInRange(T Value, T MinVal, T MaxVal)
  {
    if (MinVal < MaxVal)
      return (Value >= MinVal) && (Value <= MaxVal);
    else
      return (Value <= MinVal) && (Value >= MaxVal);
  }

  template<typename Type>
  bool IsZero(Type f, Type fEpsilon)
  {
    EZ_ASSERT_DEBUG (fEpsilon >= 0, "Epsilon may not be negative.");

    return ((f >= -fEpsilon) && (f <= fEpsilon));
  }

  template<typename Type>
  EZ_FORCE_INLINE Type Trunc(Type f)
  {
    if (f > 0)
      return Floor(f);

    return Ceil(f);
  }

  template<typename Type>
  EZ_FORCE_INLINE Type Round(Type f)
  {
    return Floor(f + (Type) 0.5);
  }

  template<typename Type>
  EZ_FORCE_INLINE Type Round(Type f, Type fRoundTo)
  {
    return Round(f / fRoundTo) * fRoundTo;
  }

  template<typename Type>
  EZ_FORCE_INLINE Type Fraction(Type f)
  {
    return (f - Trunc (f));
  }

  template<typename Type>
  inline Type SmoothStep(Type x, Type edge1, Type edge2)
  {
    x = (x - edge1) / (edge2 - edge1);

    if (x <= (Type) 0)
      return (Type) 0;
    if (x >= (Type) 1)
      return (Type) 1;

    return (x * x * ((Type) 3 - ((Type) 2 * x)));
  }
}

