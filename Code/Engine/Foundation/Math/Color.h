#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

class ezColorLinearUB;
class ezColorGammaUB;

/// \brief ezColor represents and RGBA color in linear color space. Values are stored as float, allowing HDR values and full precision color modifications.
///
/// ezColor is the central class to handle colors throughout the engine. With floating point precision it can handle any value.
/// Since it is stored in linear space, doing color transformations (e.g. adding colors or multiplying them) work as expected.
///
/// When you need to pass colors to the GPU you have multiple options.
///   * If you can spare the bandwidth, you should prefer to use floating point formats, e.g. the same as ezColor on the CPU.
///   * If you need higher precision and HDR values, you can use ezColorLinear16f as a storage format with only half the memory footprint.
///   * If you need to use preserve memory and LDR values are sufficient, you should use ezColorGammaUB. This format uses 8 Bit per pixel
///     but stores colors in Gamma space, resulting in higher precision in the range that the human eye can distinguish better.
///     However, when you store a color in Gamma space, you need to make sure to convert it back to linear space before doing ANY computations
///     with it. E.g. your shader needs to convert the color.
///   * You can also use 8 Bit per pixel with a linear color space by using ezColorLinearUB, however this may give very noticeable precision loss.
///
/// When working with color in your code, be aware to always use the correct class to handle color conversions properly.
/// E.g. when you hardcode a color in source code, you might go to a Paint program, pick a nice color and then type that value into the
/// source code. Note that ALL colors that you see on screen are implicitly in sRGB / Gamma space. That means you should do the following cast:\n
///
/// \code
///   ezColor linear = ezColorGammaUB(100, 149, 237);
/// \endcode
///
/// This will automatically convert the color from Gamma to linear space. From there on all mathematical operations are possible.
///
/// The inverse has to be done when you want to present the value of a color in a UI:
///
/// \code
///   ezColorGammaUB gamma = ezColor(0.39f, 0.58f, 0.93f);
/// \endcode
///
/// Now the integer values in \a gamma can be used to e.g. populate a color picker and the color displayed on screen will show up the same, as
/// in a gamma correct 3D rendering.
class EZ_FOUNDATION_DLL ezColor
{
public:
  EZ_DECLARE_POD_TYPE();

  // *** Predefined Colors ***
public:

  /// \brief Predefined color values.
  enum Predefined
  {
    // Predefined Colors in HTML (values given in gamma space)
    Black,      ///< #000000
    White,      ///< #FFFFFF
    Red,        ///< #FF0000
    Green,      ///< #008000
    Yellow,     ///< #FFFF00
    Blue,       ///< #0000FF
    Fuchsia,    ///< #FF00FF
    Aqua,       ///< #00FFFF
    Gray,       ///< #808080
    Lime,       ///< #00FF00
    Maroon,     ///< #800000
    Navy,       ///< #000080
    Olive,      ///< #808000
    Purple,     ///< #800080
    Silver,     ///< #C0C0C0
    Teal,       ///< #008080

    CornflowerBlue,

    ENUM_COUNT
  };

  // *** Data ***
public:

  float r;
  float g;
  float b;
  float a;

  // *** Constructors ***
public:

  /// \brief default-constructed color is uninitialized (for speed)
  ezColor(); 

  /// \brief Initializes the color with one of the predefined color values.
  ezColor(Predefined col);

  /// \brief Initializes the color with r, g, b, a. The color values must be given in a linear color space.
  ///
  /// To initialize the color from a Gamma color space, e.g. when using a color value that was determined with a color picker,
  /// use the constructor that takes a ezColorGammaUB object for initialization.
  ezColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); 

  /// \brief Initializes this color from a ezColorLinearUB object.
  ///
  /// Prefer to either use linear colors with floating point precision, or to use ezColorGammaUB for 8 bit per pixel colors in gamma space.
  ezColor(const ezColorLinearUB& cc);

  /// \brief Initializes this color from a ezColorGammaUB object.
  ///
  /// This should be the preferred method when hardcoding colors in source code.
  ezColor(const ezColorGammaUB& cc);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversion Operators/Functions ***
public:

  /// \brief Sets this color from a color that is in linear color space and given in HSV format.
  ///
  /// You should typically NOT use this functions, as most colors in HSV format are taken from some color picker,
  /// which will return a color in Gamma space.
  void FromLinearHSV(float hue, float sat, float val);

  /// \brief Converts the color part to HSV format. The HSV color will be in linear space.
  ///
  /// You should NOT use this functions when you want to display the HSV value in a UI element, as those should display colors in Gamma space.
  /// You can use this function for procedural color modifications. E.g. GetComplementaryColor() is computed by rotating the hue value 180 degree.
  /// In this case you also need to use FromLinearHSV() to convert the color back to RGB format.
  void ToLinearHSV(float& hue, float& sat, float& val) const;

  /// \brief Sets this color from a color that is in gamma space and given in HSV format.
  ///
  /// This method should be used when a color was determined through a color picker.
  void FromGammaHSV(float hue, float sat, float val);

  /// \brief Converts the color part to HSV format. The HSV color will be in gamma space.
  ///
  /// This should be used when you want to display the color as HSV in a color picker.
  void ToGammaHSV(float& hue, float& sat, float& val) const;

  /// \brief Conversion to const float*
  const float* GetData() const { return &r; } 

  /// \brief Conversion to float*
  float* GetData() { return &r; } 

  /// \brief Helper function to convert a float RGB color value from gamma space to linear color space.
  static ezVec3 GammaToLinear(const ezVec3& gamma);

  /// \brief Helper function to convert a float RGB color value from linear space to gamma color space.
  static ezVec3 LinearToGamma(const ezVec3& gamma);

  // *** Color specific functions ***
public:

  /// \brief Returns if the color is in the Range [0; 1] on all 4 channels.
  bool IsNormalized() const; 

  /// \brief Computes saturation.
  float GetSaturation() const;

  /// \brief Computes luminance.
  ///
  /// Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
  float GetLuminance() const;

  /// \brief Performs a simple (1.0 - color) inversion on all four channels.
  ///
  /// Using this function on non-normalized colors will lead to negative results.
  /// \see ezColor::IsNormalized
  ezColor GetInvertedColor() const;

  /// \brief Calculates the complementary color for this color (hue shifted by 180 degrees). The complementary color will have the same alpha.
  ezColor GetComplementaryColor() const;

  // *** Numeric properties ***
public:

  /// \brief Returns true, if any of \a r, \a g, \a b or \a a is NaN.
  bool IsNaN() const;

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const;

  // *** Operators ***
public:

  /// \brief Initializes the color with one of the predefined color values.
  void operator=(Predefined col);

  /// \brief Converts the color from ezColorLinearUB to linear float values.
  void operator= (const ezColorLinearUB& cc);

  /// \brief Converts the color from ezColorGammaUB to linear float values. Gamma is correctly converted to linear space.
  void operator= (const ezColorGammaUB& cc);

  /// \brief Adds \a rhs component-wise to this color.
  void operator+= (const ezColor& rhs);

  /// \brief Subtracts \a rhs component-wise from this vector.
  void operator-= (const ezColor& rhs);

    /// \brief Multiplies \a rhs component-wise with this color.
  void operator*= (const ezColor& rhs);

  /// \brief Multiplies all components of this color with f.
  void operator*= (float f);

  /// \brief Divides all components of this color by f.
  void operator/= (float f);

  /// \brief Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the matrix is ignored.
  ///
  /// This operation can be used to do basic color correction.
  void operator*= (const ezMat4& rhs);

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const ezColor& rhs) const;

  /// \brief Equality Check with epsilon
  bool IsEqual(const ezColor& rhs, float fEpsilon) const;

private:
  static ezColor s_PredefinedColors[(ezUInt32) Predefined::ENUM_COUNT];
};

// *** Operators ***

/// \brief Component-wise addition.
const ezColor operator+ (const ezColor& c1, const ezColor& c2);

/// \brief Component-wise subtraction.
const ezColor operator- (const ezColor& c1, const ezColor& c2);

/// \brief Component-wise multiplication.
const ezColor operator* (const ezColor& c1, const ezColor& c2);

/// \brief Returns a scaled color.
const ezColor operator* (float f, const ezColor& c);

/// \brief Returns a scaled color. Will scale all components.
const ezColor operator* (const ezColor& c, float f);

/// \brief Returns a scaled color. Will scale all components.
const ezColor operator/ (const ezColor& c, float f);

/// \brief Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the matrix is ignored.
///
/// This operation can be used to do basic color correction.
const ezColor operator* (const ezMat4& lhs, const ezColor& rhs);

/// \brief Returns true, if both colors are identical in all components.
bool operator== (const ezColor& c1, const ezColor& c2);

/// \brief Returns true, if both colors are not identical in all components.
bool operator!= (const ezColor& c1, const ezColor& c2);

EZ_CHECK_AT_COMPILETIME(sizeof(ezColor) == 16);

#include <Foundation/Math/Implementation/Color_inl.h>

