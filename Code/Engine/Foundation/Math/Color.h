#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

/// \brief A floating point color.
class ezColor
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
  ezColor(float r, float g, float b, float a = 1.0f);  // [tested]

  /// \brief Initializes the color with an vec4 type. XYZW interpreted as RGBA
  template<typename Type>
  ezColor(const ezVec4Template<Type>& v); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  // *** Conversion Operators/Functions ***
public:

  /// Conversion to ezVec4Template
  template<typename Type>
  operator ezVec4Template<Type> () const;  // [tested]

  /// Conversion to const float*
  operator const float* () const { return &r; }  // [tested]

  /// Conversion to float* - use with care!
  operator float* () { return &r; }  // [tested]

  /// \brief Returns RGB  as ezVec3.
  template<typename Type>
  ezVec3Template<Type> GetRGB() const;  // [tested]

  /// \brief Returns BGR as ezVec3.
  template<typename Type>
  ezVec3Template<Type> GetBGR() const;  // [tested]

  /// \brief Sets RGB from a ezVec3, interpreted as x=r, y=g, z=b.
  template<typename Type>
  void SetRGB(const ezVec3Template<Type>& rgb);  // [tested]

  /// \brief Sets BGR from a ezVec3, interpreted as x=b, y=g, a=r.
  template<typename Type>
  void SetBGR(const ezVec3Template<Type>& bgr);  // [tested]


  // *** Color specific functions ***
public:

  /// Returns if the color is in the Range [0; 1] on all 4 channels.
  bool IsNormalized() const;  // [tested]

  /// \brief Converts color part to HSV and returns as ezVec3.
  /// Using this function on non-normalized colors will lead to invalid results.
  /// Hue value is given by an angle in degree (0 degree - 360 degree)
  /// \see ezColor32f::IsNormalized
  template<typename Type>
  ezVec3Template<Type> ConvertToHSV() const; // [tested]

  /// \brief Sets rgb value converting a HSV color.
  /// \param hue  Hue in degree (0 degree - 360 degree).
  /// \param sat  Saturation from 0-1
  /// \param val  Value from 0-1
  template<typename Type>
  static ezColor FromHSV(Type hue, Type sat, Type val); // [tested]

  /// \copydoc ezColor32f::FromHSV
  template<typename Type>
  static ezColor FromHSV(ezVec3Template<Type> hsv) { return FromHSV(hsv.x, hsv.y, hsv.z); }

  /// \brief Computes saturation.
  float GetSaturation() const;

  /// \brief Computes luminance.
  /// Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
  float GetLuminance() const;

  /// \brief Gets inverted color (on all channels).
  /// Performs a simple 1.0-color inversion. Using this function on non-normalized colors will lead to negative results.
  /// \see ezColor32f::IsNormalized
  ezColor GetInvertedColor() const;

  /// \brief Converts color from linear to sRGB space. Will not touch alpha.
  /// Using this function on non-normalized colors will lead to negative results
  /// \see ezColor32f::IsNormalized
  ezColor ConvertLinearToSRGB() const;

  /// Converts color from sRGB to linear space. Will not touch alpha.
  ezColor ConvertSRGBToLinear() const;

  /// \todo Add nifty interpolation functions: InterpolateLinearInHSV, InterpolateLinearInRGB

  // *** Numeric properties ***
public:

  /// \brief Returns true, if any of r, g, b or a is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  // *** Operators ***
public:

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

  // *** Predefined Color values ***
public:

  static const ezColor GetRed()            { return ezColor(1.0f, 0.0f, 0.0f); }
  static const ezColor GetGreen()          { return ezColor(0.0f, 1.0f, 0.0f); }
  static const ezColor GetBlue()           { return ezColor(0.0f, 0.0f, 1.0f); }
  static const ezColor GetPink()           { return ezColor(1.0f, 0.0f, 1.0f); }
  static const ezColor GetYellow()         { return ezColor(1.0f, 1.0f, 0.0f); }
  static const ezColor GetWhite()          { return ezColor(1.0f, 1.0f, 1.0f); }
  static const ezColor GetBlack()          { return ezColor(0.0f, 0.0f, 0.0f); }
  static const ezColor GetCornflowerBlue() { return ezColor(0.39f, 0.58f, 0.93f); }  // The original!
};

// *** Operators ***

/// \brief Component-wise addition.
const ezColor operator+ (const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Component-wise subtraction.
const ezColor operator- (const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Returns a scaled color.
template<typename Type>
const ezColor operator* (Type f, const ezColor& c); // [tested]

/// \brief Returns a scaled color. Will scale all components.
template<typename Type>
const ezColor operator* (const ezColor& c, Type f); // [tested]

/// \brief Returns a scaled color. Will scale all components.
template<typename Type>
const ezColor operator/ (const ezColor& c, Type f); // [tested]

/// \brief Returns true, if both colors are identical in all components.
bool operator== (const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Returns true, if both colors are not identical in all components.
bool operator!= (const ezColor& c1, const ezColor& c2); // [tested]

#include <Foundation/Math/Implementation/Color_inl.h>

