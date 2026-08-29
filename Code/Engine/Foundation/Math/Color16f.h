#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Float16.h>

/// A 16bit per channel float color storage format.
///
/// For any calculations or conversions use ezColor.
/// \see ezColor
class EZ_FOUNDATION_DLL ezColorLinear16f
{
public:
  // Means that colors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  // *** Data ***
public:
  ezFloat16 r;
  ezFloat16 g;
  ezFloat16 b;
  ezFloat16 a;

  // *** Constructors ***
public:
  /// default-constructed color is uninitialized (for speed)
  ezColorLinear16f(); // [tested]

  /// Initializes the color with r, g, b, a
  ezColorLinear16f(ezFloat16 r, ezFloat16 g, ezFloat16 b, ezFloat16 a); // [tested]

  /// Initializes the color with ezColor
  ezColorLinear16f(const ezColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:
  /// Conversion to ezColor.
  ezColor ToLinearFloat() const; // [tested]

  /// Conversion to const ezFloat16*.
  const ezFloat16* GetData() const { return &r; }

  /// Conversion to ezFloat16* - use with care!
  ezFloat16* GetData() { return &r; }
};

#include <Foundation/Math/Implementation/Color16f_inl.h>
