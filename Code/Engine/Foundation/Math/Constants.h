#pragma once

#include <Foundation/Math/Declarations.h>

namespace ezMath
{
  /// Returns the natural constant Pi.
  template <typename TYPE>
  constexpr TYPE Pi();

  /// Returns the natural constant e.
  template <typename TYPE>
  constexpr TYPE e();

  /// Returns whether the template type supports specialized values to represent NaN.
  template <typename TYPE>
  constexpr bool SupportsNaN();

  /// Returns the value for NaN as the template type. Returns zero, if the type does not support NaN.
  ///
  /// Do not use this for comparisons, it will fail. Use it to initialize data (e.g. in debug builds), to detect uninitialized variables.
  /// Use the function IsNaN() to check whether a value is not a number.
  template <typename TYPE>
  constexpr TYPE NaN();

  /// Returns whether the template type supports specialized values to represent Infinity.
  template <typename TYPE>
  constexpr bool SupportsInfinity();

  /// Returns the value for Infinity as the template type. Returns zero, if the type does not support Infinity.
  template <typename TYPE>
  constexpr TYPE Infinity();

  /// Returns the largest possible positive value (that is not infinity).
  template <typename TYPE>
  constexpr TYPE MaxValue();

  /// Returns the smallest possible value (that is not -infinity). Usually zero or -MaxValue(). For signed integers this will be -MaxValue() - 1
  template <typename TYPE>
  constexpr TYPE MinValue();

  /// A very large value, that is slightly smaller than sqrt(MaxValue()).
  ///
  /// Useful to default initialize values, that may get squared in subsequent operations.
  template <typename TYPE>
  constexpr TYPE HighValue();

  /// The difference between 1.0 and the next representable value for the given type.
  template <typename TYPE>
  constexpr TYPE FloatEpsilon();

  template <typename TYPE>
  constexpr TYPE SmallEpsilon();

  template <typename TYPE>
  constexpr TYPE DefaultEpsilon();

  template <typename TYPE>
  constexpr TYPE LargeEpsilon();

  template <typename TYPE>
  constexpr TYPE HugeEpsilon();

  /// Returns the number of bits in the given type. Mostly useful for unsigned integer types.
  template <typename TYPE>
  constexpr ezUInt32 NumBits();
} // namespace ezMath


#include <Foundation/Math/Implementation/Constants_inl.h>
