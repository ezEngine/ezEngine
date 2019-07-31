#pragma once

#include <Foundation/Math/Declarations.h>

namespace ezMath
{
  /// \brief Returns the natural constant Pi.
  template <typename TYPE>
  constexpr TYPE Pi();

  /// \brief Returns the natural constant e.
  template <typename TYPE>
  constexpr TYPE e();

  /// \brief Returns whether the template type supports specialized values to represent NaN.
  template <typename TYPE>
  constexpr bool SupportsNaN();

  /// \brief Returns the value for NaN as the template type. Returns zero, if the type does not support NaN.
  ///
  /// Do not use this for comparisons, it will fail. Use it to initialize data (e.g. in debug builds), to detect uninitialized variables.
  /// Use the function IsNaN() to check whether a value is not a number.
  template <typename TYPE>
  constexpr TYPE NaN();

  /// \brief Returns whether the template type supports specialized values to represent Infinity.
  template <typename TYPE>
  constexpr bool SupportsInfinity();

  /// \brief Returns the value for Infinity as the template type. Returns zero, if the type does not support Infinity.
  template <typename TYPE>
  constexpr TYPE Infinity();

  /// \brief Returns the largest possible positive value (that is not infinity).
  template <typename TYPE>
  constexpr TYPE MaxValue();

  /// \brief Returns the smallest possible value (that is not -infinity). Usually zero or -MaxValue(). For signed integers this will be -MaxValue() - 1
  template <typename TYPE>
  constexpr TYPE MinValue();

  template <typename TYPE>
  constexpr TYPE SmallEpsilon();

  template <typename TYPE>
  constexpr TYPE DefaultEpsilon();

  template <typename TYPE>
  constexpr TYPE LargeEpsilon();

  template <typename TYPE>
  constexpr TYPE HugeEpsilon();
}


#include <Foundation/Math/Implementation/Constants_inl.h>
