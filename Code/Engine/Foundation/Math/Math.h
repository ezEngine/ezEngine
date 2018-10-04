#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Declarations.h>


/// \brief This namespace provides common math-functionality as functions.
///
/// It is a namespace, instead of a static class, because that allows it to be extended
/// at other locations, which is especially useful when adding custom types.
namespace ezMath
{
  /// \brief Returns whether the given value is NaN under this type.
  template <typename Type>
  constexpr static bool IsNaN(Type value) { return false; }

  /// \brief Returns whether the given value represents a finite value (i.e. not +/- Infinity and not NaN)
  template <typename Type>
  constexpr static bool IsFinite(Type value) { return true; }

  template <typename Type>
  struct BasicType
  {
    /// \brief Returns whether the templated type supports NaN, at all. Usually only true for \c float and \c double.
    constexpr static bool SupportsNaN() { return false; }

    /// \brief Returns the value for NaN as the template type. Returns zero, if the type does not support NaN.
    ///
    /// Do not use this for comparisons, it will fail. Use it to initialize data (e.g. in debug builds), to detect uninitialized variables.
    /// Use the function IsNaN() to check whether a value is not a number.
    static Type GetNaN() { return Type(0); }

    /// \brief Returns whether this templated type supports specialized values to represent Infinity.
    constexpr static bool SupportsInfinity() { return false; }

    /// \brief Returns the value for Infinity as the template type. Returns zero, if the type does not support Infinity.
    static Type GetInfinity() { return Type(0); }

    /// \brief Returns the natural constant e.
    constexpr static Type e() { return (Type)2.71828182845904; }

    /// \brief Returns the natural constant pi.
    constexpr static Type Pi() { return (Type)3.1415926535897932384626433832795; }

    /// \brief Returns the largest possible positive value (that is not infinity).
    static Type MaxValue();

    static Type SmallEpsilon() { return (Type)0.000001; }
    static Type DefaultEpsilon() { return (Type)0.00001; }
    static Type LargeEpsilon() { return (Type)0.0001; }
    static Type HugeEpsilon() { return (Type)0.001; }
  };

  /// ***** Trigonometric Functions *****

  /// \brief Takes an angle, returns its sine
  float Sin(ezAngle a); // [tested]

  /// \brief Takes an angle, returns its cosine
  float Cos(ezAngle a); // [tested]

  /// \brief Takes an angle, returns its tangent
  float Tan(ezAngle a); // [tested]

  /// \brief Returns the arcus sinus of f
  ezAngle ASin(float f); // [tested]

  /// \brief Returns the arcus cosinus of f
  ezAngle ACos(float f); // [tested]

  /// \brief Returns the arcus tangent of f
  ezAngle ATan(float f); // [tested]

  /// \brief Returns the atan2 of x and y
  ezAngle ATan2(float x, float y); // [tested]

  /// \brief Returns e^f
  float Exp(float f); // [tested]

  /// \brief Returns the logarithmus naturalis of f
  float Ln(float f); // [tested]

  /// \brief Returns log (f), to the base 2
  float Log2(float f); // [tested]

  /// \brief Returns the integral logarithm to the base 2, that comes closest to the given integer.
  ezUInt32 Log2i(ezUInt32 val); // [tested]

  /// \brief Returns log (f), to the base 10
  float Log10(float f); // [tested]

  /// \brief Returns log (f), to the base fBase
  float Log(float fBase, float f); // [tested]

  /// \brief Returns 2^f
  float Pow2(float f); // [tested]

  /// \brief Returns base^exp
  float Pow(float base, float exp); // [tested]

  /// \brief Returns 2^f
  constexpr ezInt32 Pow2(ezInt32 i); // [tested]

  /// \brief Returns base^exp
  ezInt32 Pow(ezInt32 base, ezInt32 exp); // [tested]

  /// \brief Returns f * f
  template <typename T>
  constexpr T Square(T f); // [tested]

  /// \brief Returns the square root of f
  float Sqrt(float f); // [tested]

  /// \brief Returns the square root of f
  double Sqrt(double f); // [tested]

  /// \brief Returns the n-th root of f.
  float Root(float f, float NthRoot); // [tested]

  /// \brief Returns the sign of f (i.e: -1, 1 or 0)
  template <typename T>
  constexpr T Sign(T f); // [tested]

  /// \brief Returns the absolute value of f
  template <typename T>
  constexpr T Abs(T f); // [tested]

  /// \brief Returns the smaller value, f1 or f2
  template <typename T>
  constexpr T Min(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// \brief Returns the greater value, f1 or f2
  template <typename T>
  constexpr T Max(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or ...
  template <typename T, typename... ARGS>
  constexpr T Min(T f1, T f2, ARGS... f); // [tested]

  /// \brief Clamps "value" to the range [min; max]. Returns "value", if it is inside the range already
  template <typename T>
  constexpr T Clamp(T value, T min_val, T max_val); // [tested]

  /// \brief Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  float Floor(float f); // [tested]

  /// \brief Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  float Ceil(float f); // [tested]

  /// \brief Returns a multiple of fMultiple that is smaller than f.
  float RoundDown(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is smaller than f.
  double RoundDown(double f, double fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is larger than f.
  float RoundUp(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is larger than f.
  double RoundUp(double f, double fMultiple); // [tested]

  /// \brief Returns the integer-part of f (removes the fraction).
  template <typename Type>
  Type Trunc(Type f); // [tested]

  /// \brief Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  float Round(float f); // [tested]

  /// \brief Rounds f to the next integer.
  ///
  /// If f is positive 0.5 is rounded UP (i.e. to 1), if f is negative, -0.5 is rounded DOWN (i.e. to -1).
  double Round(double f); // [tested]

  /// \brief Rounds f to the closest value of multiple.
  float RoundToMultiple(float f, float multiple);

  /// \brief Rounds f to the closest value of multiple.
  double RoundToMultiple(double f, double multiple);

  /// \brief Returns the fraction-part of f.
  template <typename Type>
  Type Fraction(Type f); // [tested]

  /// \brief Returns "value mod div" for floats. This also works with negative numbers, both for value and for div.
  float Mod(float value, float div); // [tested]

  /// \brief Returns "value mod div" for doubles. This also works with negative numbers, both for value and for div.
  double Mod(double f, double div); // [tested]

  /// \brief Returns 1 / f
  template <typename Type>
  constexpr Type Invert(Type f); // [tested]

  /// \brief Returns true, if i is an odd number
  constexpr bool IsOdd(ezInt32 i); // [tested]

  /// \brief Returns true, if i is an even number
  constexpr bool IsEven(ezInt32 i); // [tested]

  /// \brief Returns the index of the least significant bit set
  ezUInt32 FirstBitLow(ezUInt32 value);

  /// \brief Returns the index of the most significant bit set
  ezUInt32 FirstBitHigh(ezUInt32 value);

  /// \brief Swaps the values in the two variables f1 and f2
  template <typename T>
  void Swap(T& f1, T& f2); // [tested]

  /// \brief Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  T Lerp(T f1, T f2, float factor); // [tested]

  /// \brief Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  T Lerp(T f1, T f2, double factor); // [tested]

  /// \brief Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  constexpr T Step(T value, T edge); // [tested]

  /// \brief Returns 0, if value is <= edge1, 1 if value >= edge2 and the hermite interpolation in between
  template <typename Type>
  Type SmoothStep(Type value, Type edge1, Type edge2); // [tested]

  /// \brief Returns true, if there exists some x with base^x == value
  EZ_FOUNDATION_DLL bool IsPowerOf(ezInt32 value, ezInt32 base); // [tested]

  /// \brief Returns true, if there exists some x with 2^x == value
  constexpr bool IsPowerOf2(ezInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is <= value
  EZ_FOUNDATION_DLL ezUInt32 PowerOfTwo_Floor(ezUInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is >= value
  EZ_FOUNDATION_DLL ezUInt32 PowerOfTwo_Ceil(ezUInt32 value); // [tested]

  /// \brief Checks, whether fValue is in the range [fDesired - fMaxImprecision; fDesired + fMaxImprecision].
  template <typename Type>
  constexpr bool IsEqual(Type lhs, Type rhs, Type fEpsilon);

  /// \brief Checks whether the value of the first parameter lies between the value of the second and third.
  template <typename T>
  constexpr bool IsInRange(T Value, T MinVal, T MaxVal); // [tested]

  /// \brief Checks whether the given number is close to zero.
  template <typename Type>
  bool IsZero(Type f, Type fEpsilon); // [tested]

  /// \brief Converts a color value from float [0;1] range to unsigned byte [0;255] range, with proper rounding
  constexpr ezUInt8 ColorFloatToByte(float value);

  /// \brief Converts a color value from unsigned byte [0;255] range to float [0;1] range, with proper rounding
  constexpr float ColorByteToFloat(ezUInt8 value);

  /// \brief Evaluates the cubic spline defined by four control points at time \a t and returns the interpolated result.
  /// Can be used with T as float, vec2, vec3 or vec4
  template <typename T, typename T2>
  T EvaluateBezierCurve(T2 t, const T& startPoint, const T& controlPoint1, const T& controlPoint2, const T& endPoint);
} // namespace ezMath

#include <Foundation/Math/Implementation/MathDouble_inl.h>
#include <Foundation/Math/Implementation/MathFixedPoint_inl.h>
#include <Foundation/Math/Implementation/MathFloat_inl.h>
#include <Foundation/Math/Implementation/MathInt32_inl.h>
#include <Foundation/Math/Implementation/Math_inl.h>
