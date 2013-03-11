#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Declarations.h>

/// This class provides common math-functionality as static functions.
struct EZ_FOUNDATION_DLL ezMath
{
  /// Returns the natural constant e.
  static float e();

  /// Returns the natural constant pi.
  static float Pi();

  /// Returns the largest positive floating point number.
  static float FloatMax_Pos(); // [tested]

  /// Returns the largest negative floating point number.
  static float FloatMax_Neg(); // [tested]

  /// Converts an angle in degree to radians.
  static float DegToRad(float f);// [tested]

  /// Converts an angle in radians to degree.
  static float RadToDeg(float f);// [tested]

  /// Returns the a float that represents '+Infinity'.
  static float Infinity(); // [tested]

  /// Returns float NaN. Do not use this for comparisons, it will fail. Use it to initialize data (e.g. in debug builds), to detect uninitialized variables.
  static float NaN(); // [tested]

  ///  Takes an angle in degree, returns its sine
  static float SinDeg(float f); // [tested]

  ///  Takes an angle in degree, returns its cosine
  static float CosDeg(float f); // [tested]

  ///  Takes an angle in radians, returns its sine
  static float SinRad(float f); // [tested]

  ///  Takes an angle in radians, returns its cosine
  static float CosRad(float f); // [tested]

  ///  Takes an angle in degree, returns its tangent
  static float TanDeg(float f); // [tested]

  ///  Takes an angle in radians, returns its tangent
  static float TanRad(float f); // [tested]

  ///  Returns the arcus sinus of f, in degree
  static float ASinDeg(float f); // [tested]

  ///  Returns the arcus cosinus of f, in degree
  static float ACosDeg(float f); // [tested]

  ///  Returns the arcus sinus of f, in radians
  static float ASinRad(float f); // [tested]

  ///  Returns the arcus cosinus of f, in radians
  static float ACosRad(float f); // [tested]

  ///  Returns the arcus tangent of f, in degree
  static float ATanDeg(float f); // [tested]

  ///  Returns the arcus tangent of f, in radians
  static float ATanRad(float f); // [tested]

  ///  Returns the atan2 of x and y, in degree
  static float ATan2Deg(float x, float y); // [tested]

  ///  Returns the atan2 of x and y, in radians
  static float ATan2Rad(float x, float y); // [tested]

  ///  Returns e^f
  static float Exp(float f); // [tested]

  ///  Returns the logarithmus naturalis of f
  static float Ln(float f); // [tested]

  ///  Returns log (f), to the base 2
  static float Log2(float f); // [tested]

  ///  Returns the integral logarithm to the base 2, that comes closest to the given integer.
  static ezUInt32 Log2i(ezUInt32 val); // [tested]

  ///  Returns log (f), to the base 10
  static float Log10(float f); // [tested]

  ///  Returns log (f), to the base fBase
  static float Log(float fBase, float f); // [tested]

  ///  Returns 2^f
  static float Pow2(float f); // [tested]

  ///  Returns base^exp
  static float Pow(float base, float exp); // [tested]

  ///  Returns 2^f
  static ezInt32 Pow2(ezInt32 i);// [tested]

  ///  Returns base^exp
  static ezInt32 Pow(ezInt32 base, ezInt32 exp); // [tested]

  ///  Returns f * f
  template <typename T>
  static T Square(T f); // [tested]

  ///  Returns the square root of f
  static float Sqrt(float f); // [tested]

  ///  Returns the n-th root of f.
  static float Root(float f, float NthRoot); // [tested]

  ///  Returns the sign of f (ie: -1, 1 or 0)
  template <typename T>
  static T Sign(T f); // [tested]

  ///  Returns the absolute value of f
  template <typename T>
  static T Abs(T f); // [tested]

  ///  Returns the smaller value, f1 or f2
  template <typename T>
  static T Min(T f1, T f2); // [tested]

  ///  Returns the smaller value, f1 or f2 or f3
  template <typename T>
  static T Min(T f1, T f2, T f3); // [tested]

  ///  Returns the smaller value, f1 or f2 or f3 or f4
  template <typename T>
  static T Min(T f1, T f2, T f3, T f4); // [tested]

  ///  Returns the greater value, f1 or f2
  template <typename T>
  static T Max(T f1, T f2); // [tested]

  ///  Returns the smaller value, f1 or f2 or f3
  template <typename T>
  static T Max(T f1, T f2, T f3); // [tested]

  ///  Returns the smaller value, f1 or f2 or f3 or f4
  template <typename T>
  static T Max(T f1, T f2, T f3, T f4); // [tested]

  ///  Clamps "value" to the range [min; max]. Returns "value", if it is inside the range already
  template <typename T>
  static T Clamp(T value, T min_val, T max_val); // [tested]

  ///  Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  static float Floor(float f); // [tested]

  ///  Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  static float Ceil(float f); // [tested]

  /// Returns a multiple of fMultiple that is smaller than f.
  static float Floor(float f, float fMultiple); // [tested]

  /// Returns a multiple of fMultiple that is larger than f.
  static float Ceil(float f, float fMultiple); // [tested]

  /// Returns a multiple of uiMultiple that is smaller than i.
  static ezInt32 Floor(ezInt32 i, ezUInt32 uiMultiple); // [tested]

  /// Returns a multiple of uiMultiple that is larger than i.
  static ezInt32 Ceil(ezInt32 i, ezUInt32 uiMultiple); // [tested]

  ///  Returns the integer-part of f (removes the fraction).
  static float Trunc(float f); // [tested]

  ///  Rounds f to the next integer. If f is positive 0.5 is rounded UP (ie. to 1), if f is negative, -0.5 is rounded DOWN (ie. to -1).
  static float Round(float f); // [tested]

  ///  Rounds f to the closest multiple of fRoundTo.
  static float Round(float f, float fRoundTo); // [tested]

  ///  Returns the fraction-part of f.
  static float Fraction(float f); // [tested]

  ///  Converts f into an integer.
  static ezInt32 FloatToInt(float f); // [tested]

  ///  Returns "value mod div" for floats.
  static float Mod(float f, float div); // [tested]

  ///  Returns 1 / f
  static float Invert(float f); // [tested]

  ///  Returns true, if i is an odd number
  static bool IsOdd(ezInt32 i); // [tested]

  ///  Returns true, if i is an even number
  static bool IsEven(ezInt32 i); // [tested]

  ///  Swaps the values in the two variables f1 and f2
  template <typename T>
  static void Swap(T& f1, T& f2); // [tested]

  ///  Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  static T Lerp(T f1, T f2, float factor); // [tested]

  ///  Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  static T Step(T value, T edge); // [tested]

  ///  Returns 0, if value is <= edge1, 1 if value >= edge2 and the hermite interpolation in between
  static float SmoothStep(float value, float edge1, float edge2); // [tested]

  ///  Returns true, if there exists some x with base^x == value
  static bool IsPowerOf(ezInt32 value, ezInt32 base); // [tested]

  ///  Returns true, if there exists some x with 2^x == value
  static bool IsPowerOf2(ezInt32 value); // [tested]

  ///  Returns the next power-of-two that is <= value
  static ezInt32 PowerOfTwo_Floor(ezUInt32 value); // [tested]

  ///  Returns the next power-of-two that is >= value
  static ezInt32 PowerOfTwo_Ceil(ezUInt32 value); // [tested]

  ///  Checks, whether fValue is in the range [fDesired - fMaxImprecision; fDesired + fMaxImprecision].
  static bool IsFloatEqual(float lhs, float rhs, float fEpsilon = ezMath_LargeEpsilon); // [tested]

  ///  Returns true if f is NaN.
  static bool IsNaN(float f); // [tested]

  /// Returns whether f is a valid finite float (e.g. not NaN and not +/-Infinity).
  static bool IsFinite(float f); // [tested]

  /// Checks whether the value of the first parameter lies between the value of the second and third.
  template <typename T>
  static bool IsInRange(T Value, T MinVal, T MaxVal); // [tested]

  /// Checks whether the given number is close to zero.
  static bool IsZero(float f, float fEpsilon = ezMath_DefaultEpsilon); // [tested]
    
};

#include <Foundation/Math/Implementation/Math_inl.h>


