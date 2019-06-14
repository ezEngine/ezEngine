#pragma once

#include <algorithm>

namespace ezMath
{
  template <typename T>
  constexpr EZ_ALWAYS_INLINE T Square(T f)
  {
    return (f * f);
  }

  template <typename T>
  constexpr EZ_ALWAYS_INLINE T Sign(T f)
  {
    return (f < 0 ? T(-1) : f > 0 ? T(1) : 0);
  }

  template <typename T>
  constexpr EZ_ALWAYS_INLINE T Abs(T f)
  {
    return (f < 0 ? -f : f);
  }

  template <typename T>
  constexpr EZ_ALWAYS_INLINE T Min(T f1, T f2)
  {
    return (f2 < f1 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr EZ_ALWAYS_INLINE T Min(T f1, T f2, ARGS... f)
  {
    return Min(Min(f1, f2), f...);
  }

  template <typename T>
  constexpr EZ_ALWAYS_INLINE T Max(T f1, T f2)
  {
    return (f1 < f2 ? f2 : f1);
  }

  template <typename T, typename... ARGS>
  constexpr EZ_ALWAYS_INLINE T Max(T f1, T f2, ARGS... f)
  {
    return Max(Max(f1, f2), f...);
  }

  template <typename T>
  constexpr EZ_ALWAYS_INLINE T Clamp(T value, T min_val, T max_val)
  {
    return value < min_val ? min_val : (max_val < value ? max_val : value);
  }

  template <typename T>
  constexpr EZ_ALWAYS_INLINE T Saturate(T value)
  {
    return Clamp(value, T(0), T(1));
  }

  template <typename Type>
  constexpr Type Invert(Type f)
  {
    return ((Type)1) / f;
  }

  EZ_ALWAYS_INLINE ezUInt32 FirstBitLow(ezUInt32 value)
  {
    EZ_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
    _BitScanForward(&uiIndex, value);
    return uiIndex;
#elif EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
    return __builtin_ctz(value);
#else
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  EZ_ALWAYS_INLINE ezUInt32 FirstBitHigh(ezUInt32 value)
  {
    EZ_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    unsigned long uiIndex = 0;
    _BitScanReverse(&uiIndex, value);
    return uiIndex;
#elif EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
    return __builtin_clz(value);
#else
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  EZ_ALWAYS_INLINE ezUInt32 CountBits(ezUInt32 value)
  {
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
    return __popcnt(value);
#elif EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
    return __builtin_popcount(value);
#else
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  template <typename T>
  EZ_ALWAYS_INLINE void Swap(T& f1, T& f2)
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

  template <typename T>
  EZ_FORCE_INLINE T Lerp(T f1, T f2, double factor)
  {
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    EZ_ASSERT_DEBUG((factor >= -0.00001) && (factor <= 1.0 + 0.00001), "lerp: factor is not in the range [0; 1]");

    return (T)(f1 + (factor * (f2 - f1)));
  }

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  constexpr EZ_FORCE_INLINE T Step(T value, T edge)
  {
    return (value >= edge ? T(1) : T(0));
  }

  constexpr EZ_FORCE_INLINE bool IsPowerOf2(ezInt32 value) { return (value < 1) ? false : ((value & (value - 1)) == 0); }

  template <typename Type>
  constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  template <typename T>
  constexpr inline bool IsInRange(T Value, T MinVal, T MaxVal)
  {
    return MinVal < MaxVal ? (Value >= MinVal) && (Value <= MaxVal) : (Value <= MinVal) && (Value >= MaxVal);
  }

  template <typename Type>
  bool IsZero(Type f, Type fEpsilon)
  {
    EZ_ASSERT_DEBUG(fEpsilon >= 0, "Epsilon may not be negative.");

    return ((f >= -fEpsilon) && (f <= fEpsilon));
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type Trunc(Type f)
  {
    if (f > 0)
      return Floor(f);

    return Ceil(f);
  }

  template <typename Type>
  EZ_ALWAYS_INLINE Type Fraction(Type f)
  {
    return (f - Trunc(f));
  }

  template <typename Type>
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

  inline ezUInt8 ColorFloatToByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<ezUInt8>(Saturate(value) * 255.0f + 0.5f);
    }
  }

  inline ezUInt16 ColorFloatToShort(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<ezUInt16>(Saturate(value) * 65535.0f + 0.5f);
    }
  }

  inline ezInt8 ColorFloatToSignedByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 127.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<ezInt8>(value);
    }
  }

  constexpr inline float ColorByteToFloat(ezUInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 255.0f);
  }

  constexpr inline float ColorShortToFloat(ezUInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 65535.0f);
  }

  constexpr inline float ColorSignedByteToFloat(ezInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/en-us/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -128) ? -1.0f : value * (1.0f / 127.0f);
  }

  template <typename T, typename T2>
  T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const T2 mt = 1 - t;

    const T2 f1 = mt * mt * mt;
    const T2 f2 = 3 * mt * mt * t;
    const T2 f3 = 3 * mt * t * t;
    const T2 f4 = t * t * t;

    return f1 * startPoint + f2 * controlPoint1 + f3 * controlPoint2 + f4 * endPoint;
  }
} // namespace ezMath

constexpr EZ_FORCE_INLINE ezAngle ezAngle::AngleBetween(ezAngle a, ezAngle b)
{
  // taken from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return ezAngle(Pi<float>() - ezMath::Abs(ezMath::Abs(a.GetRadian() - b.GetRadian()) - Pi<float>()));
}

