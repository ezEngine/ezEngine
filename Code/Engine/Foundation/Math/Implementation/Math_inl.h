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

    return (ezUInt32)ret;
  }

  constexpr EZ_FORCE_INLINE int Pow2(int i)
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
  constexpr EZ_ALWAYS_INLINE T Square(T f)
  {
    return (f * f);
  }

  template <typename T>
  constexpr EZ_FORCE_INLINE T Sign(T f)
  {
    return (f < 0 ? T(-1) : f > 0 ? T(1) : 0);
  }

  template <typename T>
  constexpr EZ_FORCE_INLINE T Abs(T f)
  {
    return (f < 0 ? -f : f);
  }

  template <typename T>
  constexpr EZ_FORCE_INLINE T Min(T f1, T f2)
  {
    return (f2 < f1 ? f2 : f1);
  }

  template <typename T, typename ...ARGS>
  constexpr EZ_FORCE_INLINE T Min(T f1, T f2, ARGS... f)
  {
    return Min(Min(f1, f2), f...);
  }

  template <typename T>
  constexpr EZ_FORCE_INLINE T Max(T f1, T f2)
  {
    return (f1 < f2 ? f2 : f1);
  }

  template <typename T, typename ...ARGS>
  constexpr EZ_FORCE_INLINE T Max(T f1, T f2, ARGS... f)
  {
    return Max(Max(f1, f2), f...);
  }

  template <typename T>
  constexpr EZ_FORCE_INLINE T Clamp(T value, T min_val, T max_val)
  {
    return value < min_val ? min_val :
          (max_val < value ? max_val :
           value);
  }

  inline ezInt32 Floor(ezInt32 i, ezUInt32 uiMultiple)
  {
    if (i < 0)
    {
      const ezInt32 iDivides = (i + 1) / (ezInt32)uiMultiple;

      return ((iDivides - 1) * uiMultiple);
    }

    const ezInt32 iDivides = i / (ezInt32)uiMultiple;
    return (iDivides * uiMultiple);
  }

  inline ezInt32 Ceil(ezInt32 i, ezUInt32 uiMultiple)
  {
    if (i < 0)
    {
      const ezInt32 iDivides = i / (ezInt32)uiMultiple;

      return (iDivides * uiMultiple);
    }

    ezInt32 iDivides = (i - 1) / (ezInt32)uiMultiple;
    return ((iDivides + 1) * uiMultiple);
  }

  template<typename Type>
  constexpr Type Invert(Type f)
  {
    return ((Type)1) / f;
  }

  constexpr EZ_FORCE_INLINE bool IsOdd(ezInt32 i)
  {
    return ((i & 1) != 0);
  }

  constexpr EZ_FORCE_INLINE bool IsEven(ezInt32 i)
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
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    EZ_ASSERT_DEBUG((factor >= -0.00001f) && (factor <= 1.0f + 0.00001f), "lerp: factor is not in the range [0; 1]");

    return (T)(f1 + (factor * (f2 - f1)));
  }

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  constexpr EZ_FORCE_INLINE T Step(T value, T edge)
  {
    return (value >= edge ? T(1) : T(0));
  }

  constexpr EZ_FORCE_INLINE bool IsPowerOf2(ezInt32 value)
  {
    return (value < 1) ? false :
          ((value & (value - 1)) == 0);
  }

  template<typename Type>
  constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  template <typename T>
  constexpr inline bool IsInRange(T Value, T MinVal, T MaxVal)
  {
    return MinVal < MaxVal ?
      (Value >= MinVal) && (Value <= MaxVal) :
      (Value <= MinVal) && (Value >= MaxVal);
  }

  template<typename Type>
  bool IsZero(Type f, Type fEpsilon)
  {
    EZ_ASSERT_DEBUG(fEpsilon >= 0, "Epsilon may not be negative.");

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
    return (f - Trunc(f));
  }

  template<typename Type>
  inline Type SmoothStep(Type x, Type edge1, Type edge2)
  {
    const Type divider = edge2 - edge1;

    if (divider == (Type)0)
    {
      if (x >= edge2)
        return (Type)1;
      return (Type)0;
    }

    x = (x - edge1) / divider;

    if (x <= (Type)0)
      return (Type)0;
    if (x >= (Type)1)
      return (Type)1;

    return (x * x * ((Type)3 - ((Type)2 * x)));
  }

  constexpr inline ezUInt8 ColorFloatToByte(float value)
  {
    return static_cast<ezUInt8>(ezMath::Min(255.0f, ((value * 255.0f) + 0.5f)));
  }

  constexpr inline float ColorByteToFloat(ezUInt8 value)
  {
    return value * (1.0f / 255.0f);
  }


  template<typename T>
  T EvaluateBezierCurve(float t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const float mt = 1 - t;

    const float f1 = mt * mt * mt;
    const float f2 = 3 * mt * mt * t;
    const float f3 = 3 * mt * t * t;
    const float f4 = t * t * t;

    return f1 * startPoint + f2 * controlPoint1 + f3 * controlPoint2 + f4 * endPoint;
  }
}

constexpr EZ_FORCE_INLINE ezAngle ezAngle::AngleBetween(ezAngle a, ezAngle b)
{
  // taken from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return ezAngle(Pi<float>() - ezMath::Abs(ezMath::Abs(a.GetRadian() - b.GetRadian()) - Pi<float>()));
}

