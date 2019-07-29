#pragma once

namespace ezMath
{
  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float Pi()
  {
    return static_cast<float>(3.1415926535897932384626433832795f);
  }

  template <>
  constexpr double Pi()
  {
    return static_cast<double>(3.1415926535897932384626433832795);
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr float e()
  {
    return static_cast<float>(2.71828182845904);
  }

  template <>
  constexpr double e()
  {
    return static_cast<double>(2.71828182845904);
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr bool SupportsNaN()
  {
    return false;
  }

  template <>
  constexpr bool SupportsNaN<float>()
  {
    return true;
  }

  template <>
  constexpr bool SupportsNaN<double>()
  {
    return true;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr TYPE NaN()
  {
    return static_cast<TYPE>(0);
  }

  template <>
  constexpr float NaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1000 0000 0000 0000 0000 0001

    ezIntFloatUnion i2f(0x7f800042u);
    return i2f.f;
  }

  template <>
  constexpr double NaN()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // NaN = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0001

    ezInt64DoubleUnion i2f(0x7FF0000000000042ull);
    return i2f.f;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr bool SupportsInfinity()
  {
    return false;
  }

  template <>
  constexpr bool SupportsInfinity<float>()
  {
    return true;
  }

  template <>
  constexpr bool SupportsInfinity<double>()
  {
    return true;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr TYPE Infinity()
  {
    return static_cast<TYPE>(0);
  }

  template <>
  constexpr float Infinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1000 0000 0000 0000 0000 0000

    // bitwise representation of float infinity (positive)
    ezIntFloatUnion i2f(0x7f800000u);
    return i2f.f;
  }

  template <>
  constexpr double Infinity()
  {
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    // INF = 0111 1111 1111 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000 0000

    // bitwise representation of double infinity (positive)
    ezInt64DoubleUnion i2f(0x7FF0000000000000ull);

    return i2f.f;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr ezUInt8 MaxValue()
  {
    return 0xFF;
  }

  template <>
  constexpr ezUInt16 MaxValue()
  {
    return 0xFFFF;
  }

  template <>
  constexpr ezUInt32 MaxValue()
  {
    return 0xFFFFFFFFu;
  }

  template <>
  constexpr ezUInt64 MaxValue()
  {
    return 0xFFFFFFFFFFFFFFFFull;
  }

  template <>
  constexpr ezInt8 MaxValue()
  {
    return 0x7F;
  }

  template <>
  constexpr ezInt16 MaxValue()
  {
    return 0x7FFF;
  }

  template <>
  constexpr ezInt32 MaxValue()
  {
    return 0x7FFFFFFF;
  }

  template <>
  constexpr ezInt64 MaxValue()
  {
    return 0x7FFFFFFFFFFFFFFFll;
  }

  template <>
  constexpr float MaxValue()
  {
    return 3.402823465e+38F;
  }

  template <>
  constexpr double MaxValue()
  {
    return 1.7976931348623158e+307;
  }

  //////////////////////////////////////////////////////////////////////////

  template <>
  constexpr ezUInt8 MinValue()
  {
    return 0;
  }

  template <>
  constexpr ezUInt16 MinValue()
  {
    return 0;
  }

  template <>
  constexpr ezUInt32 MinValue()
  {
    return 0;
  }

  template <>
  constexpr ezUInt64 MinValue()
  {
    return 0;
  }

  template <>
  constexpr ezInt8 MinValue()
  {
    return -MaxValue<ezInt8>() - 1;
  }

  template <>
  constexpr ezInt16 MinValue()
  {
    return -MaxValue<ezInt16>() - 1;
  }

  template <>
  constexpr ezInt32 MinValue()
  {
    return -MaxValue<ezInt32>() - 1;
  }

  template <>
  constexpr ezInt64 MinValue()
  {
    return -MaxValue<ezInt64>() - 1;
  }

  template <>
  constexpr float MinValue()
  {
    return -3.402823465e+38F;
  }

  template <>
  constexpr double MinValue()
  {
    return -1.7976931348623158e+307;
  }

  //////////////////////////////////////////////////////////////////////////

  template <typename TYPE>
  constexpr TYPE SmallEpsilon()
  {
    return (TYPE)0.000001;
  }

  template <typename TYPE>
  constexpr TYPE DefaultEpsilon()
  {
    return (TYPE)0.00001;
  }

  template <typename TYPE>
  constexpr TYPE LargeEpsilon()
  {
    return (TYPE)0.0001;
  }

  template <typename TYPE>
  constexpr TYPE HugeEpsilon()
  {
    return (TYPE)0.001;
  }
  //////////////////////////////////////////////////////////////////////////

} // namespace ezMath
