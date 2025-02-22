#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

/// \brief ezColor represents an RGBA color in linear color space. Values are stored as float, allowing HDR values and full precision color
/// modifications.
///
/// ezColor is the central class to handle colors throughout the engine. With floating point precision it can handle any value, including HDR colors.
/// Since it is stored in linear space, doing color transformations (e.g. adding colors or multiplying them) work as expected.
///
/// When you need to pass colors to the GPU you have multiple options.
///   * If you can spare the bandwidth, you should prefer to use floating point formats, e.g. the same as ezColor on the CPU.
///   * If you need higher precision and HDR values, you can use ezColorLinear16f as a storage format with only half the memory footprint.
///   * If you need to preserve memory and LDR values are sufficient, you should use ezColorGammaUB. This format uses 8 Bit per pixel
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
///
///
///
/// The predefined colors can be seen at http://www.w3schools.com/colors/colors_names.asp
class EZ_FOUNDATION_DLL ezColor
{
public:
  EZ_DECLARE_POD_TYPE();

  // *** Predefined Colors ***
public:
  static const ezColor AliceBlue;            ///< #F0F8FF
  static const ezColor AntiqueWhite;         ///< #FAEBD7
  static const ezColor Aqua;                 ///< #00FFFF
  static const ezColor Aquamarine;           ///< #7FFFD4
  static const ezColor Azure;                ///< #F0FFFF
  static const ezColor Beige;                ///< #F5F5DC
  static const ezColor Bisque;               ///< #FFE4C4
  static const ezColor Black;                ///< #000000
  static const ezColor BlanchedAlmond;       ///< #FFEBCD
  static const ezColor Blue;                 ///< #0000FF
  static const ezColor BlueViolet;           ///< #8A2BE2
  static const ezColor Brown;                ///< #A52A2A
  static const ezColor BurlyWood;            ///< #DEB887
  static const ezColor CadetBlue;            ///< #5F9EA0
  static const ezColor Chartreuse;           ///< #7FFF00
  static const ezColor Chocolate;            ///< #D2691E
  static const ezColor Coral;                ///< #FF7F50
  static const ezColor CornflowerBlue;       ///< #6495ED  The original!
  static const ezColor Cornsilk;             ///< #FFF8DC
  static const ezColor Crimson;              ///< #DC143C
  static const ezColor Cyan;                 ///< #00FFFF
  static const ezColor DarkBlue;             ///< #00008B
  static const ezColor DarkCyan;             ///< #008B8B
  static const ezColor DarkGoldenRod;        ///< #B8860B
  static const ezColor DarkGray;             ///< #A9A9A9
  static const ezColor DarkGrey;             ///< #A9A9A9
  static const ezColor DarkGreen;            ///< #006400
  static const ezColor DarkKhaki;            ///< #BDB76B
  static const ezColor DarkMagenta;          ///< #8B008B
  static const ezColor DarkOliveGreen;       ///< #556B2F
  static const ezColor DarkOrange;           ///< #FF8C00
  static const ezColor DarkOrchid;           ///< #9932CC
  static const ezColor DarkRed;              ///< #8B0000
  static const ezColor DarkSalmon;           ///< #E9967A
  static const ezColor DarkSeaGreen;         ///< #8FBC8F
  static const ezColor DarkSlateBlue;        ///< #483D8B
  static const ezColor DarkSlateGray;        ///< #2F4F4F
  static const ezColor DarkSlateGrey;        ///< #2F4F4F
  static const ezColor DarkTurquoise;        ///< #00CED1
  static const ezColor DarkViolet;           ///< #9400D3
  static const ezColor DeepPink;             ///< #FF1493
  static const ezColor DeepSkyBlue;          ///< #00BFFF
  static const ezColor DimGray;              ///< #696969
  static const ezColor DimGrey;              ///< #696969
  static const ezColor DodgerBlue;           ///< #1E90FF
  static const ezColor FireBrick;            ///< #B22222
  static const ezColor FloralWhite;          ///< #FFFAF0
  static const ezColor ForestGreen;          ///< #228B22
  static const ezColor Fuchsia;              ///< #FF00FF
  static const ezColor Gainsboro;            ///< #DCDCDC
  static const ezColor GhostWhite;           ///< #F8F8FF
  static const ezColor Gold;                 ///< #FFD700
  static const ezColor GoldenRod;            ///< #DAA520
  static const ezColor Gray;                 ///< #808080
  static const ezColor Grey;                 ///< #808080
  static const ezColor Green;                ///< #008000
  static const ezColor GreenYellow;          ///< #ADFF2F
  static const ezColor HoneyDew;             ///< #F0FFF0
  static const ezColor HotPink;              ///< #FF69B4
  static const ezColor IndianRed;            ///< #CD5C5C
  static const ezColor Indigo;               ///< #4B0082
  static const ezColor Ivory;                ///< #FFFFF0
  static const ezColor Khaki;                ///< #F0E68C
  static const ezColor Lavender;             ///< #E6E6FA
  static const ezColor LavenderBlush;        ///< #FFF0F5
  static const ezColor LawnGreen;            ///< #7CFC00
  static const ezColor LemonChiffon;         ///< #FFFACD
  static const ezColor LightBlue;            ///< #ADD8E6
  static const ezColor LightCoral;           ///< #F08080
  static const ezColor LightCyan;            ///< #E0FFFF
  static const ezColor LightGoldenRodYellow; ///< #FAFAD2
  static const ezColor LightGray;            ///< #D3D3D3
  static const ezColor LightGrey;            ///< #D3D3D3
  static const ezColor LightGreen;           ///< #90EE90
  static const ezColor LightPink;            ///< #FFB6C1
  static const ezColor LightSalmon;          ///< #FFA07A
  static const ezColor LightSeaGreen;        ///< #20B2AA
  static const ezColor LightSkyBlue;         ///< #87CEFA
  static const ezColor LightSlateGray;       ///< #778899
  static const ezColor LightSlateGrey;       ///< #778899
  static const ezColor LightSteelBlue;       ///< #B0C4DE
  static const ezColor LightYellow;          ///< #FFFFE0
  static const ezColor Lime;                 ///< #00FF00
  static const ezColor LimeGreen;            ///< #32CD32
  static const ezColor Linen;                ///< #FAF0E6
  static const ezColor Magenta;              ///< #FF00FF
  static const ezColor Maroon;               ///< #800000
  static const ezColor MediumAquaMarine;     ///< #66CDAA
  static const ezColor MediumBlue;           ///< #0000CD
  static const ezColor MediumOrchid;         ///< #BA55D3
  static const ezColor MediumPurple;         ///< #9370DB
  static const ezColor MediumSeaGreen;       ///< #3CB371
  static const ezColor MediumSlateBlue;      ///< #7B68EE
  static const ezColor MediumSpringGreen;    ///< #00FA9A
  static const ezColor MediumTurquoise;      ///< #48D1CC
  static const ezColor MediumVioletRed;      ///< #C71585
  static const ezColor MidnightBlue;         ///< #191970
  static const ezColor MintCream;            ///< #F5FFFA
  static const ezColor MistyRose;            ///< #FFE4E1
  static const ezColor Moccasin;             ///< #FFE4B5
  static const ezColor NavajoWhite;          ///< #FFDEAD
  static const ezColor Navy;                 ///< #000080
  static const ezColor OldLace;              ///< #FDF5E6
  static const ezColor Olive;                ///< #808000
  static const ezColor OliveDrab;            ///< #6B8E23
  static const ezColor Orange;               ///< #FFA500
  static const ezColor OrangeRed;            ///< #FF4500
  static const ezColor Orchid;               ///< #DA70D6
  static const ezColor PaleGoldenRod;        ///< #EEE8AA
  static const ezColor PaleGreen;            ///< #98FB98
  static const ezColor PaleTurquoise;        ///< #AFEEEE
  static const ezColor PaleVioletRed;        ///< #DB7093
  static const ezColor PapayaWhip;           ///< #FFEFD5
  static const ezColor PeachPuff;            ///< #FFDAB9
  static const ezColor Peru;                 ///< #CD853F
  static const ezColor Pink;                 ///< #FFC0CB
  static const ezColor Plum;                 ///< #DDA0DD
  static const ezColor PowderBlue;           ///< #B0E0E6
  static const ezColor Purple;               ///< #800080
  static const ezColor RebeccaPurple;        ///< #663399
  static const ezColor Red;                  ///< #FF0000
  static const ezColor RosyBrown;            ///< #BC8F8F
  static const ezColor RoyalBlue;            ///< #4169E1
  static const ezColor SaddleBrown;          ///< #8B4513
  static const ezColor Salmon;               ///< #FA8072
  static const ezColor SandyBrown;           ///< #F4A460
  static const ezColor SeaGreen;             ///< #2E8B57
  static const ezColor SeaShell;             ///< #FFF5EE
  static const ezColor Sienna;               ///< #A0522D
  static const ezColor Silver;               ///< #C0C0C0
  static const ezColor SkyBlue;              ///< #87CEEB
  static const ezColor SlateBlue;            ///< #6A5ACD
  static const ezColor SlateGray;            ///< #708090
  static const ezColor SlateGrey;            ///< #708090
  static const ezColor Snow;                 ///< #FFFAFA
  static const ezColor SpringGreen;          ///< #00FF7F
  static const ezColor SteelBlue;            ///< #4682B4
  static const ezColor Tan;                  ///< #D2B48C
  static const ezColor Teal;                 ///< #008080
  static const ezColor Thistle;              ///< #D8BFD8
  static const ezColor Tomato;               ///< #FF6347
  static const ezColor Turquoise;            ///< #40E0D0
  static const ezColor Violet;               ///< #EE82EE
  static const ezColor Wheat;                ///< #F5DEB3
  static const ezColor White;                ///< #FFFFFF
  static const ezColor WhiteSmoke;           ///< #F5F5F5
  static const ezColor Yellow;               ///< #FFFF00
  static const ezColor YellowGreen;          ///< #9ACD32

  // *** Data ***
public:
  float r;
  float g;
  float b;
  float a;

  // *** Static Functions ***
public:
  /// \brief Returns a color with all four RGBA components set to Not-A-Number (NaN).
  [[nodiscard]] static ezColor MakeNaN();

  /// \brief Returns a color with all four RGBA components set to zero. This is different to ezColor::Black, which has alpha still set to 1.0.
  [[nodiscard]] static ezColor MakeZero();

  /// \brief Returns a color with the given r, g, b, a values. The values must be given in a linear color space.
  [[nodiscard]] static ezColor MakeRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f);

  // *** Constructors ***
public:
  /// \brief default-constructed color is uninitialized (for speed)
  ezColor(); // [tested]

  /// \brief Initializes the color with r, g, b, a. The color values must be given in a linear color space.
  ///
  /// To initialize the color from a Gamma color space, e.g. when using a color value that was determined with a color picker,
  /// use the constructor that takes a ezColorGammaUB object for initialization.
  constexpr ezColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  /// \brief Initializes this color from a ezColorLinearUB object.
  ///
  /// Prefer to either use linear colors with floating point precision, or to use ezColorGammaUB for 8 bit per pixel colors in gamma space.
  ezColor(const ezColorLinearUB& cc); // [tested]

  /// \brief Initializes this color from a ezColorGammaUB object.
  ///
  /// This should be the preferred method when hard-coding colors in source code.
  ezColor(const ezColorGammaUB& cc); // [tested]

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    EZ_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that "
                               "all code-paths properly initialize this object.");
  }
#endif

  /// \brief Sets the RGB components, ignores alpha.
  void SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue); // [tested]

  /// \brief Sets all four RGBA components.
  void SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha = 1.0f); // [tested]

  // *** Conversion Operators/Functions ***
public:
  /// \brief Returns a color created from the kelvin temperature. https://wikipedia.org/wiki/Color_temperature
  /// Originally inspired from https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
  /// But with heavy modification to better fit the mapping shown out in https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
  /// Physically accurate clipping points are 6580K for Red and 6560K for G and B. but approximated to 6570k for all to give a better mapping.
  [[nodiscard]] static ezColor MakeFromKelvin(ezUInt32 uiKelvin);

  /// \brief Sets this color from a HSV (hue, saturation, value) format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  [[nodiscard]] static ezColor MakeHSV(float fHue, float fSat, float fVal); // [tested]

  /// \brief Converts the color part to HSV format.
  ///
  /// \a hue is in range [0; 360], \a sat and \a val are in range [0; 1]
  void GetHSV(float& out_fHue, float& out_fSat, float& out_fValue) const; // [tested]

  /// \brief Conversion to const float*
  const float* GetData() const { return &r; }

  /// \brief Conversion to float*
  float* GetData() { return &r; }

  /// \brief Returns the 4 color values packed in an ezVec4
  const ezVec4 GetAsVec4() const;

  /// \brief Helper function to convert a float color value from gamma space to linear color space.
  static float GammaToLinear(float fGamma); // [tested]
  /// \brief Helper function to convert a float color value from linear space to gamma color space.
  static float LinearToGamma(float fGamma); // [tested]

  /// \brief Helper function to convert a float RGB color value from gamma space to linear color space.
  static ezVec3 GammaToLinear(const ezVec3& vGamma); // [tested]
  /// \brief Helper function to convert a float RGB color value from linear space to gamma color space.
  static ezVec3 LinearToGamma(const ezVec3& vGamma); // [tested]

  // *** Color specific functions ***
public:
  /// \brief Returns if the color is in the Range [0; 1] on all 4 channels.
  bool IsNormalized() const; // [tested]

  /// \brief Calculates the average of the RGB channels.
  float CalcAverageRGB() const;

  /// \brief Computes saturation.
  float GetSaturation() const; // [tested]

  /// \brief Computes the perceived luminance. Assumes linear color space (http://en.wikipedia.org/wiki/Luminance_%28relative%29).
  float GetLuminance() const; /// [tested]

  /// \brief Performs a simple (1.0 - color) inversion on all four channels.
  ///
  /// Using this function on non-normalized colors will lead to negative results.
  /// \see ezColor IsNormalized
  ezColor GetInvertedColor() const; // [tested]

  /// \brief Calculates the complementary color for this color (hue shifted by 180 degrees). The complementary color will have the same alpha.
  ezColor GetComplementaryColor() const; // [tested]

  /// \brief Multiplies the given factor into red, green and blue, but not alpha.
  void ScaleRGB(float fFactor);

  /// \brief Multiplies the given factor into red, green, blue and also alpha.
  void ScaleRGBA(float fFactor);

  /// \brief Returns 1 for an LDR color (all ´RGB components < 1). Otherwise the value of the largest component. Ignores alpha.
  float ComputeHdrMultiplier() const;

  /// \brief Returns the base-2 logarithm of ComputeHdrMultiplier().
  /// 0 for LDR colors, +1, +2, etc. for HDR colors.
  float ComputeHdrExposureValue() const;

  /// \brief Raises 2 to the power \a ev and multiplies RGB with that factor.
  void ApplyHdrExposureValue(float fEv);

  /// \brief If this is an HDR color, the largest component value is used to normalize RGB to LDR range. Alpha is unaffected.
  void NormalizeToLdrRange();

  /// \brief Returns a darker color by converting the color to HSV, dividing the *value* by fFactor and converting it back.
  ezColor GetDarker(float fFactor = 2.0f) const;

  // *** Numeric properties ***
public:
  /// \brief Returns true, if any of \a r, \a g, \a b or \a a is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  // *** Operators ***
public:
  /// \brief Converts the color from ezColorLinearUB to linear float values.
  void operator=(const ezColorLinearUB& cc); // [tested]

  /// \brief Converts the color from ezColorGammaUB to linear float values. Gamma is correctly converted to linear space.
  void operator=(const ezColorGammaUB& cc); // [tested]

  /// \brief Adds \a rhs component-wise to this color.
  void operator+=(const ezColor& rhs); // [tested]

  /// \brief Subtracts \a rhs component-wise from this vector.
  void operator-=(const ezColor& rhs); // [tested]

  /// \brief Multiplies \a rhs component-wise with this color.
  void operator*=(const ezColor& rhs); // [tested]

  /// \brief Multiplies all components of this color with f.
  void operator*=(float f); // [tested]

  /// \brief Divides all components of this color by f.
  void operator/=(float f); // [tested]

  /// \brief Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
  /// matrix is ignored.
  ///
  /// This operation can be used to do basic color correction.
  void operator*=(const ezMat4& rhs); // [tested]

  /// \brief Equality Check (bitwise). Only compares RGB, ignores Alpha.
  bool IsIdenticalRGB(const ezColor& rhs) const; // [tested]

  /// \brief Equality Check (bitwise). Compares all four components.
  bool IsIdenticalRGBA(const ezColor& rhs) const; // [tested]

  /// \brief Equality Check with epsilon. Only compares RGB, ignores Alpha.
  bool IsEqualRGB(const ezColor& rhs, float fEpsilon) const; // [tested]

  /// \brief Equality Check with epsilon. Compares all four components.
  bool IsEqualRGBA(const ezColor& rhs, float fEpsilon) const; // [tested]

  /// \brief Returns the current color but with changes the alpha value to the given value.
  ezColor WithAlpha(float fAlpha) const;

  /// \brief Packs the 4 color values as uint8 into a single uint32 with A in the least significant bits and R in the most significant ones.
  [[nodiscard]] ezUInt32 ToRGBA8() const;

  /// \brief Packs the 4 color values as uint8 into a single uint32 with R in the least significant bits and A in the most significant ones.
  [[nodiscard]] ezUInt32 ToABGR8() const;
};

// *** Operators ***

/// \brief Component-wise addition.
const ezColor operator+(const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Component-wise subtraction.
const ezColor operator-(const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Component-wise multiplication.
const ezColor operator*(const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Returns a scaled color.
const ezColor operator*(float f, const ezColor& c); // [tested]

/// \brief Returns a scaled color. Will scale all components.
const ezColor operator*(const ezColor& c, float f); // [tested]

/// \brief Returns a scaled color. Will scale all components.
const ezColor operator/(const ezColor& c, float f); // [tested]

/// \brief Transforms the RGB components by the matrix. Alpha has no influence on the computation and will stay unmodified. The fourth row of the
/// matrix is ignored.
///
/// This operation can be used to do basic color correction.
const ezColor operator*(const ezMat4& lhs, const ezColor& rhs); // [tested]

/// \brief Returns true, if both colors are identical in all components.
bool operator==(const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Returns true, if both colors are not identical in all components.
bool operator!=(const ezColor& c1, const ezColor& c2); // [tested]

/// \brief Strict weak ordering. Useful for sorting colors into a map.
bool operator<(const ezColor& c1, const ezColor& c2); // [tested]

static_assert(sizeof(ezColor) == 16);

#include <Foundation/Math/Implementation/Color_inl.h>
