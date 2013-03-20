#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Declarations.h>

/// \brief This class provides common math-functionality as static functions.
struct EZ_FOUNDATION_DLL ezMath
{
  /// \brief Returns the natural constant e.
  static float e();

  /// \brief Returns the natural constant pi.
  static float Pi();

  /// \brief Returns the largest positive floating point number.
  static float FloatMax_Pos(); // [tested]

  /// \brief Returns the largest negative floating point number.
  static float FloatMax_Neg(); // [tested]

  /// \brief Converts an angle in degree to radians.
  static float DegToRad(float f);// [tested]

  /// \brief Converts an angle in radians to degree.
  static float RadToDeg(float f);// [tested]

  /// \brief Returns the a float that represents '+Infinity'.
  static float Infinity(); // [tested]

  /// \brief Returns float NaN. Do not use this for comparisons, it will fail. Use it to initialize data (e.g. in debug builds), to detect uninitialized variables.
  static float NaN(); // [tested]

  /// \brief Takes an angle in degree, returns its sine
  static float SinDeg(float f); // [tested]

  /// \brief Takes an angle in degree, returns its cosine
  static float CosDeg(float f); // [tested]

  /// \brief Takes an angle in radians, returns its sine
  static float SinRad(float f); // [tested]

  /// \brief Takes an angle in radians, returns its cosine
  static float CosRad(float f); // [tested]

  /// \brief Takes an angle in degree, returns its tangent
  static float TanDeg(float f); // [tested]

  /// \brief Takes an angle in radians, returns its tangent
  static float TanRad(float f); // [tested]

  /// \brief Returns the arcus sinus of f, in degree
  static float ASinDeg(float f); // [tested]

  /// \brief Returns the arcus cosinus of f, in degree
  static float ACosDeg(float f); // [tested]

  /// \brief Returns the arcus sinus of f, in radians
  static float ASinRad(float f); // [tested]

  /// \brief Returns the arcus cosinus of f, in radians
  static float ACosRad(float f); // [tested]

  /// \brief Returns the arcus tangent of f, in degree
  static float ATanDeg(float f); // [tested]

  /// \brief Returns the arcus tangent of f, in radians
  static float ATanRad(float f); // [tested]

  /// \brief Returns the atan2 of x and y, in degree
  static float ATan2Deg(float x, float y); // [tested]

  /// \brief Returns the atan2 of x and y, in radians
  static float ATan2Rad(float x, float y); // [tested]

  /// \brief Returns e^f
  static float Exp(float f); // [tested]

  /// \brief Returns the logarithmus naturalis of f
  static float Ln(float f); // [tested]

  /// \brief Returns log (f), to the base 2
  static float Log2(float f); // [tested]

  /// \brief Returns the integral logarithm to the base 2, that comes closest to the given integer.
  static ezUInt32 Log2i(ezUInt32 val); // [tested]

  /// \brief Returns log (f), to the base 10
  static float Log10(float f); // [tested]

  /// \brief Returns log (f), to the base fBase
  static float Log(float fBase, float f); // [tested]

  /// \brief Returns 2^f
  static float Pow2(float f); // [tested]

  /// \brief Returns base^exp
  static float Pow(float base, float exp); // [tested]

  /// \brief Returns 2^f
  static ezInt32 Pow2(ezInt32 i);// [tested]

  /// \brief Returns base^exp
  static ezInt32 Pow(ezInt32 base, ezInt32 exp); // [tested]

  /// \brief Returns f * f
  template <typename T>
  static T Square(T f); // [tested]

  /// \brief Returns the square root of f
  static float Sqrt(float f); // [tested]

  /// \brief Returns the n-th root of f.
  static float Root(float f, float NthRoot); // [tested]

  /// \brief Returns the sign of f (ie: -1, 1 or 0)
  template <typename T>
  static T Sign(T f); // [tested]

  /// \brief Returns the absolute value of f
  template <typename T>
  static T Abs(T f); // [tested]

  /// \brief Returns the smaller value, f1 or f2
  template <typename T>
  static T Min(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or f3
  template <typename T>
  static T Min(T f1, T f2, T f3); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or f3 or f4
  template <typename T>
  static T Min(T f1, T f2, T f3, T f4); // [tested]

  /// \brief Returns the greater value, f1 or f2
  template <typename T>
  static T Max(T f1, T f2); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or f3
  template <typename T>
  static T Max(T f1, T f2, T f3); // [tested]

  /// \brief Returns the smaller value, f1 or f2 or f3 or f4
  template <typename T>
  static T Max(T f1, T f2, T f3, T f4); // [tested]

  /// \brief Clamps "value" to the range [min; max]. Returns "value", if it is inside the range already
  template <typename T>
  static T Clamp(T value, T min_val, T max_val); // [tested]

  /// \brief Returns the next smaller integer, closest to f. Also the SMALLER value, if f is negative.
  static float Floor(float f); // [tested]

  /// \brief Returns the next higher integer, closest to f. Also the HIGHER value, if f is negative.
  static float Ceil(float f); // [tested]

  /// \brief Returns a multiple of fMultiple that is smaller than f.
  static float Floor(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of fMultiple that is larger than f.
  static float Ceil(float f, float fMultiple); // [tested]

  /// \brief Returns a multiple of uiMultiple that is smaller than i.
  static ezInt32 Floor(ezInt32 i, ezUInt32 uiMultiple); // [tested]

  /// \brief Returns a multiple of uiMultiple that is larger than i.
  static ezInt32 Ceil(ezInt32 i, ezUInt32 uiMultiple); // [tested]

  /// \brief Returns the integer-part of f (removes the fraction).
  static float Trunc(float f); // [tested]

  /// \brief Rounds f to the next integer. If f is positive 0.5 is rounded UP (ie. to 1), if f is negative, -0.5 is rounded DOWN (ie. to -1).
  static float Round(float f); // [tested]

  /// \brief Rounds f to the closest multiple of fRoundTo.
  static float Round(float f, float fRoundTo); // [tested]

  /// \brief Returns the fraction-part of f.
  static float Fraction(float f); // [tested]

  /// \brief Converts f into an integer.
  static ezInt32 FloatToInt(float f); // [tested]

  /// \brief Returns "value mod div" for floats.
  static float Mod(float f, float div); // [tested]

  /// \brief Returns 1 / f
  static float Invert(float f); // [tested]

  /// \brief Returns true, if i is an odd number
  static bool IsOdd(ezInt32 i); // [tested]

  /// \brief Returns true, if i is an even number
  static bool IsEven(ezInt32 i); // [tested]

  /// \brief Swaps the values in the two variables f1 and f2
  template <typename T>
  static void Swap(T& f1, T& f2); // [tested]

  /// \brief Returns the linear interpolation of f1 and f2. factor is a value between 0 and 1.
  template <typename T>
  static T Lerp(T f1, T f2, float factor); // [tested]

  /// \brief Returns 0, if value < edge, and 1, if value >= edge.
  template <typename T>
  static T Step(T value, T edge); // [tested]

  /// \brief Returns 0, if value is <= edge1, 1 if value >= edge2 and the hermite interpolation in between
  static float SmoothStep(float value, float edge1, float edge2); // [tested]

  /// \brief Returns true, if there exists some x with base^x == value
  static bool IsPowerOf(ezInt32 value, ezInt32 base); // [tested]

  /// \brief Returns true, if there exists some x with 2^x == value
  static bool IsPowerOf2(ezInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is <= value
  static ezInt32 PowerOfTwo_Floor(ezUInt32 value); // [tested]

  /// \brief Returns the next power-of-two that is >= value
  static ezInt32 PowerOfTwo_Ceil(ezUInt32 value); // [tested]

  /// \brief Checks, whether fValue is in the range [fDesired - fMaxImprecision; fDesired + fMaxImprecision].
  static bool IsFloatEqual(float lhs, float rhs, float fEpsilon = ezMath_LargeEpsilon); // [tested]

  /// \brief Returns true if f is NaN.
  static bool IsNaN(float f); // [tested]

  /// \brief Returns whether f is a valid finite float (e.g. not NaN and not +/-Infinity).
  static bool IsFinite(float f); // [tested]

  /// \brief Checks whether the value of the first parameter lies between the value of the second and third.
  template <typename T>
  static bool IsInRange(T Value, T MinVal, T MaxVal); // [tested]

  /// \brief Checks whether the given number is close to zero.
  static bool IsZero(float f, float fEpsilon = ezMath_DefaultEpsilon); // [tested]
    
};

#include <Foundation/Math/Implementation/Math_inl.h>


