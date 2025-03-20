#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Constants.h>
#include <Foundation/Math/Declarations.h>


/// \brief This namespace provides common math-functionality as functions.
///
/// It is a namespace, instead of a static class, because that allows it to be extended
/// at other locations, which is especially useful when adding custom types.
namespace ezMath
{
  /// \brief Returns whether the given value is NaN under this type.
  template <typename Type>
  constexpr static bool IsNaN(Type value)
  {
    EZ_IGNORE_UNUSED(value);
    return false;
  }

  /// \brief Returns whether the given value represents a finite value (i.e. not +/- Infinity and not NaN)
  template <typename Type>
  constexpr static bool IsFinite(Type value)
  {
    EZ_IGNORE_UNUSED(value);
    return true;
  }

  /// ***** Trigonometric Functions *****

  /// \brief Takes an angle, returns its sine
  [[nodiscard]] float Sin(ezAngle a); // [tested]

  /// \brief Takes an angle, returns its cosine
  [[nodiscard]] float Cos(ezAngle a); // [tested]

  /// \brief Takes an angle, returns its tangent
  [[nodiscard]] float Tan(ezAngle a); // [tested]

  /// \brief Returns the arcus sinus of f
  [[nodiscard]] ezAngle ASin(float f); // [tested]

  /// \brief Returns the arcus cosinus of f
  [[nodiscard]] ezAngle ACos(float f); // [tested]

  /// \brief Returns the arcus tangent of f
  [[nodiscard]] ezAngle ATan(float f); // [tested]

  /// \brief Returns the atan2 of x and y
  [[nodiscard]] ezAngle ATan2(float y, float x); // [tested]

  /// \brief Returns e^f
  [[nodiscard]] float Exp(float f); // [tested]

  /// \brief Returns the logarithmus naturalis of f
  [[nodiscard]] float Ln(float f); // [tested]

  /// \brief Returns log (f), to the base 2
  [[nodiscard]] float Log2(float f); // [tested]

  /// \brief Returns the integral logarithm to the base 2, that comes closest to the given integer.
  [[nodiscard]] ezUInt32 Log2i(ezUInt32 uiVal); // [tested]

  /// \brief Returns log (f), to the base 10
  [[nodiscard]] float Log10(float f); // [tested]

  /// \brief Returns log (f), to the base fBase
  [[nodiscard]] float Log(float fBase, float f); // [tested]

  /// \brief Returns 2^f
  [[nodiscard]] float Pow2(float f); // [tested]

  /// \brief Returns base^exp
  [[nodiscard]] float Pow(float fBase, float fExp); // [tested]

  /// \brief Returns 2^f
  [[nodiscard]] constexpr ezInt32 Pow2(ezInt32 i); // [tested]

  /// \brief Returns base^exp
  [[nodiscard]] ezInt32 Pow(ezInt32 iBase, ezInt32 iExp); // [tested]

  /// \brief Returns f * f
  template <typename T>
  [[nodiscard]] constexpr T Square(T f); // [tested]

  /// \brief Returns the square root of f
  [[nodiscard]] float Sqrt(float f); // [tested]

  /// \brief Returns the square root of f
  [[nodiscard]] double Sqrt(double f); // [tested]

  /// \brief Returns the n-th root of f.
  [[nodiscard]] float Root(float f, float fNthRoot); // [tested]

  /// \brief Returns the sign of f (i.e: -1, 1 or 0)
  template <typename T>
  [[nodiscard]] constexpr T Sign(T f); // [tested]

  /// \brief Returns the absolute value of f
  template <typename T>
  [[nodiscard]] constexpr T Abs(T f); // [tested]

  /// \brief Returns the smaller value, f1 or f2
  template <typename T>
  [[nodiscard]] constexpr T Min(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  [[nodiscard]] constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// \brief Returns the greater value, f1 or f2
  template <typename T>
  [[nodiscard]] constexpr T Max(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  [[nodiscard]] constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// \brief Clamps "value" to the range [min; max]. Returns "value", if it is inside the range already
  template <typename T>
  [[nodiscard]] constexpr T Clamp(T value, T min_val, T max_val); // [tested]

  /// \brief Wraps uiValue around the maximum value, so that it stays within the range [0; uiExcludedMaxValue-1].
  ///
  /// Ie a value of uiExcludedMaxValue would be wrapped to 0, and (uiExcludedMaxValue+1) to 1, etc.
  /// A value of 0 for uiExcludedMaxValue is invalid and results in a division by zero error.
  [[nodiscard]] constexpr ezUInt32 WrapUInt(ezUInt32 uiValue, ezUInt32 uiExcludedMaxValue); // [tested]

  /// \brief Wraps iValue around the maximum value, so that it stays within the range [0; uiExcludedMaxValue-1].
  ///
  /// Ie a value of uiExcludedMaxValue would be wrapped to 0, and (uiExcludedMaxValue+1) to 1, etc.
  /// Negative values are wrapped back around to a large value, ie -1 would be wrapped to (uiExcludedMaxValue-1).
  /// A value of 0 for uiExcludedMaxValue is invalid and results in a division by zero error.
  [[nodiscard]] constexpr ezInt32 WrapInt(ezInt32 iValue, ezUInt32 uiExcludedMaxValue); // [tested]

  /// \brief Wraps iValue around the minimum and maximum value, so that it stays within the range [iMinValue; iExcludedMaxValue-1].
  ///
  /// Ie a value of iExcludedMaxValue would be wrapped to iMinValue, and (iExcludedMaxValue+1) to (iMinValue+1), etc.
  /// Values below iMinValue are wrapped back around to a large value, ie (iMinValue-1) would be wrapped to (iExcludedMaxValue-1).
  ///
  /// Both iMinValue and iExcludedMaxValue can be negative, but iMinValue has to be strictly smaller than iExcludedMaxValue.
  [[nodiscard]] constexpr ezInt32 WrapInt(ezInt32 iValue, ezInt32 iMinValue, ezInt32 iExcludedMaxValue); // [tested]

  /// \brief Wraps a float value around to stay within the [0; 1] range.
  ///
  /// Wrapping happens in both positive and negative direction. Ie -0.1f will be wrapped to 0.9f and 1.1f will be wrapped to 0.1f.
  /// Note that here the value 1.0f is included in the range. Only values larger than 1.0f get wrapped back to zero.
  /// Therefore it is different to what 'Fraction' would return.
  [[nodiscard]] float WrapFloat01(float fValue); // [tested]

  /// \brief Wraps a float value around to stay within the [min; max] range.
  ///
  /// Both fMinValue and fMaxValue are inclusive.
  /// Both values may be negative, but fMinValue has to be strictly smaller than fMaxValue.
  [[nodiscard]] float WrapFloat(float fValue, float fMinValue, float fMaxValue); // [tested]

  /// \brief Clamps "value" to the range [0; 1]. Returns "value", if it is inside the range already
  template <typename T>
  [[nodiscard]] constexpr T Saturate(T value); // [tested]

  /// \brief Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  [[nodiscard]] float Floor(float f); // [tested]

  /// \brief Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  ///
  /// This function is identical to 'Floor()' except that it already returns the result cast to an int.
  [[nodiscard]] ezInt32 FloorToInt(float f); // [tested]

  /// \brief Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  [[nodiscard]] float Ceil(float f); // [tested]

  /// \brief Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  ///
  /// This function is identical to 'Ceil()' except that it already returns the result cast to an int.
  [[nodiscard]] ezInt32 CeilToInt(float f); // [tested]

  /// \brief Returns a multiple of fMultiple that is smaller than f.
  [[nodiscard]] float RoundDown(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is smaller than f.
  [[nodiscard]] double RoundDown(double f, double fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is larger than f.
  [[nodiscard]] float RoundUp(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is larger than f.
  [[nodiscard]] double RoundUp(double f, double fMultiple); // [tested]

  /// \brief Returns the integer-part of f (removes the fraction).
  template <typename Type>
  [[nodiscard]] Type Trunc(Type f); // [tested]

  /// \brief Casts the float to an integer, removes the fractional part
  ///
  /// \sa Trunc, Round, Floor, Ceil
  [[nodiscard]] constexpr ezInt32 FloatToInt(float value);

  // There is a compiler bug in VS 2019 targeting 32-bit that causes an internal compiler error when casting double to long long.
  // FloatToInt(double) is not available on these version of the MSVC compiler.
#if EZ_DISABLED(EZ_PLATFORM_ARCH_X86) || (_MSC_VER <= 1916)
  /// \brief Casts the float to an integer, removes the fractional part
  ///
  /// \sa Trunc, Round, Floor, Ceil
  [[nodiscard]] constexpr ezInt64 FloatToInt(double value);
#endif

  /// \brief Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  [[nodiscard]] float Round(float f); // [tested]

  /// \brief Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  ///
  /// This function is identical to 'Round()' except that it already returns the result cast to an int.
  [[nodiscard]] ezInt32 RoundToInt(float f); // [tested]

  /// \brief Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  [[nodiscard]] double Round(double f); // [tested]

  /// \brief Rounds f to the closest value of multiple.
  [[nodiscard]] float RoundToMultiple(float f, float fMultiple);

  /// \brief Rounds f to the closest value of multiple.
  [[nodiscard]] double RoundToMultiple(double f, double fMultiple);

  /// \brief Returns the fraction-part of f.
  template <typename Type>
  [[nodiscard]] Type Fraction(Type f); // [tested]

  /// \brief Returns "value mod div" for floats. This also works with negative numbers, both for value and for div.
  [[nodiscard]] float Mod(float value, float fDiv); // [tested]

  /// \brief Returns "value mod div" for doubles. This also works with negative numbers, both for value and for div.
  [[nodiscard]] double Mod(double f, double fDiv); // [tested]

  /// \brief Returns 1 / f
  template <typename Type>
  [[nodiscard]] constexpr Type Invert(Type f); // [tested]

  /// \brief Returns a multiple of the given multiple that is larger than or equal to value.
  [[nodiscard]] constexpr ezInt32 RoundUp(ezInt32 value, ezUInt16 uiMultiple); // [tested]

  /// \brief Returns a multiple of the given multiple that is smaller than or equal to value.
  [[nodiscard]] constexpr ezInt32 RoundDown(ezInt32 value, ezUInt16 uiMultiple); // [tested]

  /// \brief Returns a multiple of the given multiple that is greater than or equal to value.
  [[nodiscard]] constexpr ezUInt32 RoundUp(ezUInt32 value, ezUInt16 uiMultiple); // [tested]

  /// \brief Returns a multiple of the given multiple that is smaller than or equal to value.
  [[nodiscard]] constexpr ezUInt32 RoundDown(ezUInt32 value, ezUInt16 uiMultiple); // [tested]

  /// \brief Returns true, if i is an odd number
  [[nodiscard]] constexpr bool IsOdd(ezInt32 i); // [tested]

  /// \brief Returns true, if i is an even number
  [[nodiscard]] constexpr bool IsEven(ezInt32 i); // [tested]

  /// \brief Returns the index of the least significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] ezUInt32 FirstBitLow(ezUInt32 uiBitmask); // [tested]

  /// \brief Returns the index of the least significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] ezUInt32 FirstBitLow(ezUInt64 uiBitmask); // [tested]

  /// \brief Returns the index of the most significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] ezUInt32 FirstBitHigh(ezUInt32 uiBitmask); // [tested]

  /// \brief Returns the index of the most significant bit set
  ///
  /// Asserts that bitmask is not 0.
  [[nodiscard]] ezUInt32 FirstBitHigh(ezUInt64 uiBitmask); // [tested]

  /// Returns the number of zeros at the end (least significant part) of a bitmask.
  ///
  /// E.g.
  /// 0b0111 -> 0
  /// 0b0110 -> 1
  /// 0b0100 -> 2
  /// Returns 32 when the input is 0
  [[nodiscard]] ezUInt32 CountTrailingZeros(ezUInt32 uiBitmask); // [tested]

  /// \brief 64 bit overload for CountTrailingZeros()
  [[nodiscard]] ezUInt32 CountTrailingZeros(ezUInt64 uiBitmask); // [tested]

  /// Returns the number of zeros at the start (most significant part) of a bitmask.
  ///
  /// E.g.
  /// 0b0111 -> 29
  /// 0b0011 -> 30
  /// 0b0001 -> 31
  /// 0b0000 -> 32
  /// Returns 32 when the input is 0
  [[nodiscard]] ezUInt32 CountLeadingZeros(ezUInt32 uiBitmask); // [tested]

  /// \brief Returns the number of bits set
  [[nodiscard]] ezUInt32 CountBits(ezUInt32 value);

  /// \brief Returns the number of bits set
  [[nodiscard]] ezUInt32 CountBits(ezUInt64 value);

  /// \brief Creates a bitmask in which the low N bits are set. For example for N=5, this would be '0000 ... 0001 1111'
  ///
  /// For N >= 32 all bits will be set.
  template <typename Type>
  [[nodiscard]] constexpr Type Bitmask_LowN(ezUInt32 uiNumBitsToSet);

  /// \brief Creates a bitmask in which the high N bits are set. For example for N=5, this would be '1111 1000 ... 0000'
  ///
  /// For N >= 32 all bits will be set.
  template <typename Type>
  [[nodiscard]] constexpr Type Bitmask_HighN(ezUInt32 uiNumBitsToSet);

  /// \brief Swaps the values in the two variables f1 and f2
  template <typename T>
  void Swap(T& ref_f1, T& ref_f2); // [tested]

  /// \brief Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  [[nodiscard]] T Lerp(T f1, T f2, float fFactor); // [tested]

  /// \brief Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  [[nodiscard]] T Lerp(T f1, T f2, double fFactor); // [tested]

  /// \brief Returns the interpolation factor such that Lerp(fMin, fMax, factor) == fValue.
  template <typename T>
  [[nodiscard]] constexpr float Unlerp(T fMin, T fMax, T fValue); // [tested]

  /// \brief Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  [[nodiscard]] constexpr T Step(T value, T edge); // [tested]

  /// \brief Returns 0, if value is <= edge1, 1 if value >= edge2 and the hermite interpolation in between
  template <typename Type>
  [[nodiscard]] Type SmoothStep(Type value, Type edge1, Type edge2); // [tested]

  /// \brief Returns 0, if value is <= edge1, 1 if value >= edge2 and the second order hermite interpolation in between
  template <typename Type>
  [[nodiscard]] Type SmootherStep(Type value, Type edge1, Type edge2); // [tested]

  /// \brief Returns true, if there exists some x with base^x == value
  [[nodiscard]] EZ_FOUNDATION_DLL bool IsPowerOf(ezInt32 value, ezInt32 iBase); // [tested]

  /// \brief Returns true, if there exists some x with 2^x == value
  [[nodiscard]] constexpr bool IsPowerOf2(ezInt32 value); // [tested]

  /// \brief Returns true, if there exists some x with 2^x == value
  [[nodiscard]] constexpr bool IsPowerOf2(ezUInt32 value); // [tested]

  /// \brief Returns true, if there exists some x with 2^x == value
  [[nodiscard]] constexpr bool IsPowerOf2(ezUInt64 value); // [tested]

  /// \brief Returns the next power-of-two that is <= value
  [[nodiscard]] EZ_FOUNDATION_DLL ezUInt32 PowerOfTwo_Floor(ezUInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is <= value
  [[nodiscard]] EZ_FOUNDATION_DLL ezUInt64 PowerOfTwo_Floor(ezUInt64 value); // [tested]

  /// \brief Returns the next power-of-two that is >= value
  [[nodiscard]] EZ_FOUNDATION_DLL ezUInt32 PowerOfTwo_Ceil(ezUInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is >= value
  [[nodiscard]] EZ_FOUNDATION_DLL ezUInt64 PowerOfTwo_Ceil(ezUInt64 value); // [tested]

  /// \brief Returns the greatest common divisor.
  [[nodiscard]] EZ_FOUNDATION_DLL ezUInt32 GreatestCommonDivisor(ezUInt32 a, ezUInt32 b); // [tested]

  /// \brief Checks, whether fValue is in the range [fDesired - fMaxImprecision; fDesired + fMaxImprecision].
  template <typename Type>
  [[nodiscard]] constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon);

  /// \brief Checks whether the value of the first parameter lies between the value of the second and third.
  template <typename T>
  [[nodiscard]] constexpr bool IsInRange(T value, T minVal, T maxVal); // [tested]

  /// \brief Checks whether the given number is close to zero.
  template <typename Type>
  [[nodiscard]] bool IsZero(Type f, Type fEpsilon); // [tested]

  /// \brief Converts a color value from float [0;1] range to unsigned int with the given number of bits, with proper rounding
  template <ezUInt32 NumBits>
  [[nodiscard]] ezUInt32 ColorFloatToUnsignedInt(float value);

  /// \brief Converts a color value from float [0;1] range to unsigned byte [0;255] range, with proper rounding
  [[nodiscard]] ezUInt8 ColorFloatToByte(float value); // [tested]

  /// \brief Converts a color value from float [0;1] range to unsigned short [0;65535] range, with proper rounding
  [[nodiscard]] ezUInt16 ColorFloatToShort(float value); // [tested]

  /// \brief Converts a color value from float [-1;1] range to signed byte [-127;127] range, with proper rounding
  [[nodiscard]] ezInt8 ColorFloatToSignedByte(float value); // [tested]

  /// \brief Converts a color value from float [-1;1] range to signed short [-32767;32767] range, with proper rounding
  [[nodiscard]] ezInt16 ColorFloatToSignedShort(float value); // [tested]

  /// \brief Converts a color value from unsigned int with the given numer of bits to float [0;1] range, with proper rounding
  template <ezUInt32 NumBits>
  [[nodiscard]] constexpr float ColorUnsignedIntToFloat(ezUInt32 value);

  /// \brief Converts a color value from unsigned byte [0;255] range to float [0;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorByteToFloat(ezUInt8 value); // [tested]

  /// \brief Converts a color value from unsigned short [0;65535] range to float [0;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorShortToFloat(ezUInt16 value); // [tested]

  /// \brief Converts a color value from signed byte [-128;127] range to float [-1;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorSignedByteToFloat(ezInt8 value); // [tested]

  /// \brief Converts a color value from signed short [-32768;32767] range to float [0;1] range, with proper rounding
  [[nodiscard]] constexpr float ColorSignedShortToFloat(ezInt16 value); // [tested]

  /// \brief Evaluates the cubic spline defined by four control points at time \a t and returns the interpolated result.
  /// Can be used with T as float, vec2, vec3 or vec4
  template <typename T, typename T2>
  [[nodiscard]] T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint);

  /// \brief Evaluates the derivative of a cubic spline defined by four control points at time \a t and returns the interpolated result.
  /// Can be used with T as float, vec2, vec3 or vec4
  template <typename T, typename T2>
  [[nodiscard]] T EvaluateBezierCurveDerivative(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint);

  /// \brief out_Result = \a a * \a b. If an overflow happens, EZ_FAILURE is returned.
  EZ_FOUNDATION_DLL ezResult TryMultiply32(ezUInt32& out_uiResult, ezUInt32 a, ezUInt32 b, ezUInt32 c = 1, ezUInt32 d = 1); // [tested]

  /// \brief returns \a a * \a b. If an overflow happens, the program is terminated.
  [[nodiscard]] EZ_FOUNDATION_DLL ezUInt32 SafeMultiply32(ezUInt32 a, ezUInt32 b, ezUInt32 c = 1, ezUInt32 d = 1);

  /// \brief out_Result = \a a * \a b. If an overflow happens, EZ_FAILURE is returned.
  EZ_FOUNDATION_DLL ezResult TryMultiply64(ezUInt64& out_uiResult, ezUInt64 a, ezUInt64 b, ezUInt64 c = 1, ezUInt64 d = 1); // [tested]

  /// \brief returns \a a * \a b. If an overflow happens, the program is terminated.
  [[nodiscard]] EZ_FOUNDATION_DLL ezUInt64 SafeMultiply64(ezUInt64 a, ezUInt64 b, ezUInt64 c = 1, ezUInt64 d = 1);

  /// \brief Checks whether the given 64bit value actually fits into size_t, If it doesn't EZ_FAILURE is returned.
  ezResult TryConvertToSizeT(size_t& out_uiResult, ezUInt64 uiValue); // [tested]

  /// \brief Checks whether the given 64bit value actually fits into size_t, If it doesn't the program is terminated.
  [[nodiscard]] EZ_FOUNDATION_DLL size_t SafeConvertToSizeT(ezUInt64 uiValue);

  /// \brief If 'value' is not-a-number (NaN) 'fallback' is returned, otherwise 'value' is passed through unmodified.
  [[nodiscard]] EZ_FOUNDATION_DLL float ReplaceNaN(float fValue, float fFallback); // [tested]

  /// \brief If 'value' is not-a-number (NaN) 'fallback' is returned, otherwise 'value' is passed through unmodified.
  [[nodiscard]] EZ_FOUNDATION_DLL double ReplaceNaN(double fValue, double fFallback); // [tested]

  /// \brief Combines the two 32 bit uint values into one 64 bit value.
  [[nodiscard]] constexpr ezUInt64 MakeUInt64(ezUInt32 uiHigh32, ezUInt32 uiLow32);

} // namespace ezMath

#include <Foundation/Math/Implementation/MathDouble_inl.h>
#include <Foundation/Math/Implementation/MathFixedPoint_inl.h>
#include <Foundation/Math/Implementation/MathFloat_inl.h>
#include <Foundation/Math/Implementation/MathInt32_inl.h>
#include <Foundation/Math/Implementation/Math_inl.h>
