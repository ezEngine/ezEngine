#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Color.h>

/// \brief A 8bit per channel unsigned normalized (values interpreted as 0-1) color storage format.
/// 
/// For any calculations or conversions use ezColor.
/// \see ezColor
class EZ_FOUNDATION_DLL ezColor8UNorm
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

  /// \brief Initializes the color with ezColor.
  /// Assumes that the given color is normalized.
  /// \see ezColor::IsNormalized
  ezColor8UNorm(const ezColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:

  /// \brief Conversion to ezColor.
  operator const ezColor () const; // [tested]

  /// \brief Conversion to const ezUInt8*.
  const ezUInt8* GetData() const { return &r; } // [tested]

  /// \brief Conversion to ezUInt8* - use with care!
  ezUInt8* GetData() { return &r; } // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezColor8UNorm& rhs) const; // [tested]
};

/// \brief Returns true, if both colors are identical in all components.
bool operator== (const ezColor8UNorm& c1, const ezColor8UNorm& c2); // [tested]

/// \brief Returns true, if both colors are not identical in all components.
bool operator!= (const ezColor8UNorm& c1, const ezColor8UNorm& c2); // [tested]

EZ_CHECK_AT_COMPILETIME(sizeof(ezColor8UNorm) == 4);




/// \brief The same as ezColor8UNorm but with BGRA data layout which is needed for handling certain image formats.
/// 
/// For any calculations or conversions use ezColor.
/// \see ezColor
class EZ_FOUNDATION_DLL ezColorBgra8UNorm
{
public:
  // Means that colors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();


  // *** Data ***
public:

  ezUInt8 b;
  ezUInt8 g;
  ezUInt8 r;
  ezUInt8 a;

  // *** Constructors ***
public:

  /// \brief default-constructed color is uninitialized (for speed)
  ezColorBgra8UNorm(); // [tested]

  /// \brief Initializes the color with b, g, r, a
  ezColorBgra8UNorm(ezUInt8 b, ezUInt8 g, ezUInt8 r, ezUInt8 a = 255); // [tested]

  /// \brief Initializes the color with ezColor.
  /// Assumes that the given color is normalized.
  /// \see ezColor::IsNormalized
  ezColorBgra8UNorm(const ezColor& color); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Functions ***
public:

  /// \brief Conversion to ezColor.
  operator const ezColor () const; // [tested]

  /// \brief Conversion to const ezUInt8*.
  const ezUInt8* GetData() const { return &b; } // [tested]

  /// \brief Conversion to ezUInt8* - use with care!
  ezUInt8* GetData() { return &b; } // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezColorBgra8UNorm& rhs) const; // [tested]
};

/// \brief Returns true, if both colors are identical in all components.
bool operator== (const ezColorBgra8UNorm& c1, const ezColorBgra8UNorm& c2); // [tested]

/// \brief Returns true, if both colors are not identical in all components.
bool operator!= (const ezColorBgra8UNorm& c1, const ezColorBgra8UNorm& c2); // [tested]

EZ_CHECK_AT_COMPILETIME(sizeof(ezColorBgra8UNorm) == 4);



#include <Foundation/Math/Implementation/Color8UNorm_inl.h>


