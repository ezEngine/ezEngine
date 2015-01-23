#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

class ezColorLinearUB;
class ezColorGammaUB;

class EZ_FOUNDATION_DLL ezColor
{
public:
  // Means that colors can be copied using memcpy instead of copy construction.
  EZ_DECLARE_POD_TYPE();

  // *** Data ***
public:

  float r;
  float g;
  float b;
  float a;

  // *** Constructors ***
public:

  /// \brief default-constructed color is uninitialized (for speed)
  ezColor();  // [tested]

  /// \brief Initializes the color with r, g, b, a
  ezColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f);  // [tested]

  ezColor(const ezColorLinearUB& cc);

  ezColor(const ezColorGammaUB& cc);

  // no copy-constructor and operator= since the default-generated ones will be faster

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversion Operators/Functions ***
public:

  /// \brief Sets this color from a color that is in linear color space and given in HSV format.
  /// You should typically NOT use this functions, as most colors in HSV format are taken from some color picker, which will return a color in Gamma space.
  void FromLinearHSV(float hue, float sat, float val); // [tested]

  /// \brief Converts the color part to HSV format. The HSV color will be in linear space.
  /// You should NOT use this functions when you want to display the HSV value in a UI element, as those should display colors in Gamma space.
  /// You can use this function for procedural color modifications. E.g. GetComplementaryColor() is computed by rotating the hue value 180 degree.
  /// In this case you also need to use FromLinearHSV() to convert the color back to RGB format.
  void ToLinearHSV(float& hue, float& sat, float& val) const; // [tested]

  void FromGammaHSV(float hue, float sat, float val);

  void ToGammaHSV(float& hue, float& sat, float& val) const;

  /// Conversion to const float*
  const float* GetData() const { return &r; }  // [tested]

  /// Conversion to float* - use with care!
  float* GetData() { return &r; }  // [tested]

  static ezVec3 GammaToLinear(const ezVec3& gamma);
  static ezVec3 LinearToGamma(const ezVec3& gamma);


  // *** Color specific functions ***
public:

  /// Returns if the color is in the Range [0; 1] on all 4 channels.
  bool IsNormalized() const;  // [tested]


  /// \brief Computes saturation.
  float GetSaturation() const;

  /// \brief Computes luminance.
  /// Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
  float GetLuminance() const;

  /// \brief Gets inverted color (on all channels).
  /// Performs a simple 1.0-color inversion. Using this function on non-normalized colors will lead to negative results.
  /// \see ezColor::IsNormalized
  ezColor GetInvertedColor() const;

  /// Calculates the complementary color for this color (hue shifted by 180 degrees), complementary color will have the same alpha.
  ezColor GetComplementaryColor() const;

  // *** Numeric properties ***
public:

  /// \brief Returns true, if any of r, g, b or a is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  // *** Operators ***
public:

  void operator= (const ezColorLinearUB& cc);

  void operator= (const ezColorGammaUB& cc);

  /// \brief Adds cc component-wise to this color.
  void operator+= (const ezColor& cc); // [tested]

  /// \brief Subtracts cc component-wise from this vector.
  void operator-= (const ezColor& cc); // [tested]

  /// \brief Multiplies all components of this color with f.
  void operator*= (float f); // [tested]

  /// \brief Divides all components of this color by f.
  void operator/= (float f); // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezColor& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const ezColor& rhs, float fEpsilon) const; // [tested]
};

// *** Operators ***

/// \brief Component-wise addition.
const ezColor operator+ (const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Component-wise subtraction.
const ezColor operator- (const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Returns a scaled color.
const ezColor operator* (float f, const ezColor& c); // [tested]

/// \brief Returns a scaled color. Will scale all components.
const ezColor operator* (const ezColor& c, float f); // [tested]

/// \brief Returns a scaled color. Will scale all components.
const ezColor operator/ (const ezColor& c, float f); // [tested]

/// \brief Returns true, if both colors are identical in all components.
bool operator== (const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Returns true, if both colors are not identical in all components.
bool operator!= (const ezColor& c1, const ezColor& c2); // [tested]

EZ_CHECK_AT_COMPILETIME(sizeof(ezColor) == 16);

#include <Foundation/Math/Implementation/Color_inl.h>

