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
    return (f < 0 ? T(-1) : f > 0 ? T(1)
                                  : 0);
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
    static_assert(std::is_floating_point_v<Type>);

    return ((Type)1) / f;
  }

  EZ_ALWAYS_INLINE ezUInt32 FirstBitLow(ezUInt32 value)
  {
    EZ_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if EZ_ENABLED(EZ_COMPILER_MSVC)
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

  EZ_ALWAYS_INLINE ezUInt32 FirstBitLow(ezUInt64 value)
  {
    EZ_ASSERT_DEBUG(value != 0, "FirstBitLow is undefined for 0");

#if __castxml__
    return 0;
#elif EZ_ENABLED(EZ_COMPILER_MSVC)
    unsigned long uiIndex = 0;
#  if EZ_ENABLED(EZ_PLATFORM_64BIT)

    _BitScanForward64(&uiIndex, value);
#  else
    uint32_t lower = static_cast<uint32_t>(value);
    unsigned char returnCode = _BitScanForward(&uiIndex, lower);
    if (returnCode == 0)
    {
      uint32_t upper = static_cast<uint32_t>(value >> 32);
      returnCode = _BitScanForward(&uiIndex, upper);
      if (returnCode > 0) // Only can happen in Release build when EZ_ASSERT_DEBUG(value != 0) would fail.
      {
        uiIndex += 32;    // Add length of lower to index.
      }
    }
#  endif
    return uiIndex;
#elif EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
    return __builtin_ctzll(value);
#else
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  EZ_ALWAYS_INLINE ezUInt32 FirstBitHigh(ezUInt32 value)
  {
    EZ_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if EZ_ENABLED(EZ_COMPILER_MSVC)
    unsigned long uiIndex = 0;
    _BitScanReverse(&uiIndex, value);
    return uiIndex;
#elif EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
    return 31 - __builtin_clz(value);
#else
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  EZ_ALWAYS_INLINE ezUInt32 FirstBitHigh(ezUInt64 value)
  {
    EZ_ASSERT_DEBUG(value != 0, "FirstBitHigh is undefined for 0");

#if __castxml__
    return 0;
#elif EZ_ENABLED(EZ_COMPILER_MSVC)
    unsigned long uiIndex = 0;
#  if EZ_ENABLED(EZ_PLATFORM_64BIT)
    _BitScanReverse64(&uiIndex, value);
#  else
    uint32_t upper = static_cast<uint32_t>(value >> 32);
    unsigned char returnCode = _BitScanReverse(&uiIndex, upper);
    if (returnCode == 0)
    {
      uint32_t lower = static_cast<uint32_t>(value);
      returnCode = _BitScanReverse(&uiIndex, lower);
    }
    else
    {
      uiIndex += 32; // Add length of upper to index.
    }
#  endif
    return uiIndex;
#elif EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
    return 63 - __builtin_clzll(value);
#else
    EZ_ASSERT_NOT_IMPLEMENTED;
    return 0;
#endif
  }

  EZ_ALWAYS_INLINE ezUInt32 CountTrailingZeros(ezUInt32 uiBitmask)
  {
    return (uiBitmask == 0) ? 32 : FirstBitLow(uiBitmask);
  }

  EZ_ALWAYS_INLINE ezUInt32 CountTrailingZeros(ezUInt64 uiBitmask)
  {
    const ezUInt32 numLow = CountTrailingZeros(static_cast<ezUInt32>(uiBitmask & 0xFFFFFFFF));
    const ezUInt32 numHigh = CountTrailingZeros(static_cast<ezUInt32>((uiBitmask >> 32u) & 0xFFFFFFFF));

    return (numLow == 32) ? (32 + numHigh) : numLow;
  }

  EZ_ALWAYS_INLINE ezUInt32 CountLeadingZeros(ezUInt32 uiBitmask)
  {
    return (uiBitmask == 0) ? 32 : (31u - FirstBitHigh(uiBitmask));
  }


  EZ_ALWAYS_INLINE ezUInt32 CountBits(ezUInt32 value)
  {
#if EZ_ENABLED(EZ_COMPILER_MSVC) && (EZ_ENABLED(EZ_PLATFORM_ARCH_X86) || (EZ_ENABLED(EZ_PLATFORM_ARCH_ARM) && EZ_ENABLED(EZ_PLATFORM_32BIT)))
#  if EZ_ENABLED(EZ_PLATFORM_ARCH_X86)
    return __popcnt(value);
#  else
    return _CountOneBits(value);
#  endif
#elif EZ_ENABLED(EZ_COMPILER_GCC) || EZ_ENABLED(EZ_COMPILER_CLANG)
    return __builtin_popcount(value);
#else
    value = value - ((value >> 1) & 0x55555555u);
    value = (value & 0x33333333u) + ((value >> 2) & 0x33333333u);
    return ((value + (value >> 4) & 0xF0F0F0Fu) * 0x1010101u) >> 24;
#endif
  }

  EZ_ALWAYS_INLINE ezUInt32 CountBits(ezUInt64 value)
  {
    ezUInt32 result = 0;
    result += CountBits(ezUInt32(value));
    result += CountBits(ezUInt32(value >> 32));
    return result;
  }

  template <typename Type>
  EZ_ALWAYS_INLINE constexpr Type Bitmask_LowN(ezUInt32 uiNumBitsToSet)
  {
    return (uiNumBitsToSet >= sizeof(Type) * 8) ? ~static_cast<Type>(0) : ((static_cast<Type>(1) << uiNumBitsToSet) - static_cast<Type>(1));
  }

  template <typename Type>
  EZ_ALWAYS_INLINE constexpr Type Bitmask_HighN(ezUInt32 uiNumBitsToSet)
  {
    return (uiNumBitsToSet == 0) ? 0 : ~static_cast<Type>(0) << ((sizeof(Type) * 8) - ezMath::Min<ezUInt32>(uiNumBitsToSet, sizeof(Type) * 8));
  }

  template <typename T>
  EZ_ALWAYS_INLINE void Swap(T& ref_f1, T& ref_f2)
  {
    std::swap(ref_f1, ref_f2);
  }

  template <typename T>
  EZ_FORCE_INLINE T Lerp(T f1, T f2, float fFactor)
  {
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    EZ_ASSERT_DEBUG((fFactor >= -0.00001f) && (fFactor <= 1.0f + 0.00001f), "lerp: factor is not in the range [0; 1]");

    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  template <typename T>
  EZ_FORCE_INLINE T Lerp(T f1, T f2, double fFactor)
  {
    // value is not included in format string, to prevent requirement on FormatString.h, to break #include cycles
    EZ_ASSERT_DEBUG((fFactor >= -0.00001) && (fFactor <= 1.0 + 0.00001), "lerp: factor is not in the range [0; 1]");

    return (T)(f1 + (fFactor * (f2 - f1)));
  }

  template <typename T>
  EZ_FORCE_INLINE constexpr float Unlerp(T fMin, T fMax, T fValue)
  {
    return static_cast<float>(fValue - fMin) / static_cast<float>(fMax - fMin);
  }

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  constexpr EZ_FORCE_INLINE T Step(T value, T edge)
  {
    return (value >= edge ? T(1) : T(0));
  }

  constexpr EZ_FORCE_INLINE bool IsPowerOf2(ezInt32 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  constexpr EZ_FORCE_INLINE bool IsPowerOf2(ezUInt32 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  constexpr EZ_FORCE_INLINE bool IsPowerOf2(ezUInt64 value)
  {
    return (value < 1) ? false : ((value & (value - 1)) == 0);
  }

  template <typename Type>
  constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon)
  {
    return ((rhs >= lhs - fEpsilon) && (rhs <= lhs + fEpsilon));
  }

  template <typename T>
  constexpr inline bool IsInRange(T value, T minVal, T maxVal)
  {
    return minVal < maxVal ? (value >= minVal) && (value <= maxVal) : (value <= minVal) && (value >= maxVal);
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
      return (x >= edge2) ? 1 : 0;
    }

    x = Saturate((x - edge1) / divider);

    return (x * x * ((Type)3 - ((Type)2 * x)));
  }

  template <typename Type>
  inline Type SmootherStep(Type x, Type edge1, Type edge2)
  {
    const Type divider = edge2 - edge1;

    if (divider == (Type)0)
    {
      return (x >= edge2) ? 1 : 0;
    }

    x = Saturate((x - edge1) / divider);

    return (x * x * x * (x * ((Type)6 * x - (Type)15) + (Type)10));
  }

  template <ezUInt32 NumBits>
  inline ezUInt32 ColorFloatToUnsignedInt(float value)
  {
    constexpr float fMaxValue = static_cast<float>(ezMath::Bitmask_LowN<ezUInt32>(NumBits));

    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      return static_cast<ezUInt32>(Saturate(value) * fMaxValue + 0.5f);
    }
  }

  EZ_ALWAYS_INLINE ezUInt8 ColorFloatToByte(float value)
  {
    return static_cast<ezUInt8>(ColorFloatToUnsignedInt<8>(value));
  }

  EZ_ALWAYS_INLINE ezUInt16 ColorFloatToShort(float value)
  {
    return static_cast<ezUInt16>(ColorFloatToUnsignedInt<16>(value));
  }

  inline ezInt8 ColorFloatToSignedByte(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
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

  inline ezInt16 ColorFloatToSignedShort(float value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    if (IsNaN(value))
    {
      return 0;
    }
    else
    {
      value = Clamp(value, -1.0f, 1.0f) * 32767.0f;
      if (value >= 0.0f)
      {
        value += 0.5f;
      }
      else
      {
        value -= 0.5f;
      }
      return static_cast<ezInt16>(value);
    }
  }

  constexpr inline float ColorByteToFloat(ezUInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 255.0f);
  }

  constexpr inline float ColorShortToFloat(ezUInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return value * (1.0f / 65535.0f);
  }

  constexpr inline float ColorSignedByteToFloat(ezInt8 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -128) ? -1.0f : value * (1.0f / 127.0f);
  }

  constexpr inline float ColorSignedShortToFloat(ezInt16 value)
  {
    // Implemented according to
    // https://docs.microsoft.com/windows/desktop/direct3d10/d3d10-graphics-programming-guide-resources-data-conversion
    return (value == -32768) ? -1.0f : value * (1.0f / 32767.0f);
  }

  template <typename T, typename T2>
  T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const T2 mt = 1 - t;
    const T2 mt2 = mt * mt;
    const T2 t2 = t * t;

    const T2 f1 = mt2 * mt;
    const T2 f2 = 3 * mt2 * t;
    const T2 f3 = 3 * mt * t2;
    const T2 f4 = t * t2;

    return f1 * startPoint + f2 * controlPoint1 + f3 * controlPoint2 + f4 * endPoint;
  }

  template <typename T, typename T2>
  T EvaluateBezierCurveDerivative(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint)
  {
    const T2 mt = 1 - t;

    const T2 f1 = 3 * mt * mt;
    const T2 f2 = 6 * mt * t;
    const T2 f3 = 3 * t * t;

    return f1 * (controlPoint1 - startPoint) + f2 * (controlPoint2 - controlPoint1) + f3 * (endPoint - controlPoint2);
  }
} // namespace ezMath

constexpr EZ_FORCE_INLINE ezAngle ezAngle::AngleBetween(ezAngle a, ezAngle b)
{
  // taken from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return ezAngle(Pi<float>() - ezMath::Abs(ezMath::Abs(a.GetRadian() - b.GetRadian()) - Pi<float>()));
}

constexpr EZ_FORCE_INLINE ezInt32 ezMath::FloatToInt(float value)
{
  return static_cast<ezInt32>(value);
}

#if EZ_DISABLED(EZ_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
constexpr EZ_FORCE_INLINE ezInt64 ezMath::FloatToInt(double value)
{
  return static_cast<ezInt64>(value);
}
#endif

EZ_ALWAYS_INLINE ezResult ezMath::TryConvertToSizeT(size_t& out_uiResult, ezUInt64 uiValue)
{
#if EZ_ENABLED(EZ_PLATFORM_32BIT)
  if (uiValue <= MaxValue<size_t>())
  {
    out_uiResult = static_cast<size_t>(uiValue);
    return EZ_SUCCESS;
  }

  return EZ_FAILURE;
#else
  out_uiResult = static_cast<size_t>(uiValue);
  return EZ_SUCCESS;
#endif
}

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
EZ_ALWAYS_INLINE size_t ezMath::SafeConvertToSizeT(ezUInt64 uiValue)
{
  return uiValue;
}
#endif

EZ_ALWAYS_INLINE constexpr ezUInt32 ezMath::WrapUInt(ezUInt32 uiValue, ezUInt32 uiExcludedMaxValue)
{
  return uiValue % uiExcludedMaxValue;
}

EZ_ALWAYS_INLINE constexpr ezInt32 ezMath::WrapInt(ezInt32 iValue, ezUInt32 uiExcludedMaxValue)
{
  const ezInt32 wrapped = (iValue % static_cast<ezInt32>(uiExcludedMaxValue));
  return wrapped >= 0 ? wrapped : (wrapped + uiExcludedMaxValue);
}

EZ_ALWAYS_INLINE constexpr ezInt32 ezMath::WrapInt(ezInt32 iValue, ezInt32 iMinValue, ezInt32 iExcludedMaxValue)
{
  EZ_ASSERT_DEBUG(iMinValue < iExcludedMaxValue, "Invalid range to wrap integer around.");
  return iMinValue + WrapInt(iValue - iMinValue, static_cast<ezUInt32>(iExcludedMaxValue - iMinValue));
}

EZ_ALWAYS_INLINE float ezMath::WrapFloat01(float fValue)
{
  if (fValue < 0.0f)
  {
    return fValue + Ceil(-fValue);
  }
  else if (fValue > 1.0f)
  {
    return fValue - Ceil(fValue - 1.0f);
  }

  return fValue;
}

EZ_ALWAYS_INLINE float ezMath::WrapFloat(float fValue, float fMinValue, float fMaxValue)
{
  const float range = fMaxValue - fMinValue;
  return fMinValue + WrapFloat01((fValue - fMinValue) / range) * range;
}

EZ_ALWAYS_INLINE constexpr ezUInt64 ezMath::MakeUInt64(ezUInt32 uiHigh32, ezUInt32 uiLow32)
{
  return (static_cast<ezUInt64>(uiHigh32) << 32) | static_cast<ezUInt64>(uiLow32);
}
