#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Color.h>

/// \brief A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format.
/// 
/// For any calculations or conversions use ezColor.
/// \see ezColor
class ezColor8UNorm
{
public:
  // Means that colors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();


  // *** Data ***
public:

  ezUInt8 r;
  ezUInt8 g;
  ezUInt8 b;
  ezUInt8 a;

  // *** Constructors ***
public:

  /// \brief default-constructed color is uninitialized (for speed)
  ezColor8UNorm(); // [tested]

  /// \brief Initializes the color with r, g, b, a
  ezColor8UNorm(ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a = 255); // [tested]

  /// \brief Initializes the color with ezColor32f
  /// Assumes that the given color is normalized.
  /// \see ezColor32f::IsNormalized
  ezColor8UNorm(const ezColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:

  /// \brief Conversion to ezColor32f.
  operator ezColor () const; // [tested]

  /// \brief Conversion to const ezUInt8*.
  operator const ezUInt8* () const { return &r; } // [tested]

  /// \brief Conversion to ezUInt8* - use with care!
  operator ezUInt8* () { return &r; } // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezColor8UNorm& rhs) const; // [tested]
};

/// \brief Returns true, if both colors are identical in all components.
bool operator== (const ezColor8UNorm& c1, const ezColor8UNorm& c2); // [tested]

/// \brief Returns true, if both colors are not identical in all components.
bool operator!= (const ezColor8UNorm& c1, const ezColor8UNorm& c2); // [tested]

#include <Foundation/Math/Implementation/Color8UNorm_inl.h>


