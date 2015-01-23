#pragma once

#include <Foundation/Math/Float16.h>
#include <Foundation/Math/Color.h>

/// \brief A 16bit per channel float color storage format.
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

  /// \brief default-constructed color is uninitialized (for speed)
  ezColorLinear16f(); // [tested]

  /// \brief Initializes the color with r, g, b, a
  ezColorLinear16f(ezFloat16 r, ezFloat16 g, ezFloat16 b, ezFloat16 a); // [tested]

  /// \brief Initializes the color with ezColor
  ezColorLinear16f(const ezColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:

  /// \brief Conversion to ezColor.
  ezColor ToLinearFloat() const; // [tested]

  /// \brief Conversion to const ezFloat16*.
  const ezFloat16* GetData() const { return &r; }

  /// \brief Conversion to ezFloat16* - use with care!
  ezFloat16* GetData() { return &r; }
};

#include <Foundation/Math/Implementation/Color16f_inl.h>


