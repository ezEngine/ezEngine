#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec4.h>

class ezColorLinearUB;
class ezColorGammaUB;

/// \brief ezColor represents and RGBA color in linear color space. Values are stored as float, allowing HDR values and full precision color modifications.
///
/// ezColor is the central class to handle colors throughout the engine. With floating point precision it can handle any value, including HDR colors.
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

  /// \brief Predefined color values. You can preview the color palette at http://www.w3schools.com/cssref/css_colornames.asp
  enum Predefined
  {
    // 140 CSS Color Names (values given in Gamma space)
    AliceBlue,              ///< #F0F8FF
    AntiqueWhite,           ///< #FAEBD7
    Aqua,                   ///< #00FFFF
    Aquamarine,             ///< #7FFFD4
    Azure,                  ///< #F0FFFF
    Beige,                  ///< #F5F5DC
    Bisque,                 ///< #FFE4C4
    Black,                  ///< #000000
    BlanchedAlmond,         ///< #FFEBCD
    Blue,                   ///< #0000FF
    BlueViolet,             ///< #8A2BE2
    Brown,                  ///< #A52A2A
    BurlyWood,              ///< #DEB887
    CadetBlue,              ///< #5F9EA0
    Chartreuse,             ///< #7FFF00
    Chocolate,              ///< #D2691E
    Coral,                  ///< #FF7F50
    CornflowerBlue,         ///< #6495ED  The original!
    Cornsilk,               ///< #FFF8DC
    Crimson,                ///< #DC143C
    Cyan,                   ///< #00FFFF
    DarkBlue,               ///< #00008B
    DarkCyan,               ///< #008B8B
    DarkGoldenRod,          ///< #B8860B
    DarkGray,               ///< #A9A9A9
    DarkGreen,              ///< #006400
    DarkKhaki,              ///< #BDB76B
    DarkMagenta,            ///< #8B008B
    DarkOliveGreen,         ///< #556B2F
    DarkOrange,             ///< #FF8C00
    DarkOrchid,             ///< #9932CC
    DarkRed,                ///< #8B0000
    DarkSalmon,             ///< #E9967A
    DarkSeaGreen,           ///< #8FBC8F
    DarkSlateBlue,          ///< #483D8B
    DarkSlateGray,          ///< #2F4F4F
    DarkTurquoise,          ///< #00CED1
    DarkViolet,             ///< #9400D3
    DeepPink,               ///< #FF1493
    DeepSkyBlue,            ///< #00BFFF
    DimGray,                ///< #696969
    DodgerBlue,             ///< #1E90FF
    FireBrick,              ///< #B22222
    FloralWhite,            ///< #FFFAF0
    ForestGreen,            ///< #228B22
    Fuchsia,                ///< #FF00FF
    Gainsboro,              ///< #DCDCDC
    GhostWhite,             ///< #F8F8FF
    Gold,                   ///< #FFD700
    GoldenRod,              ///< #DAA520
    Gray,                   ///< #808080
    Green,                  ///< #008000
    GreenYellow,            ///< #ADFF2F
    HoneyDew,               ///< #F0FFF0
    HotPink,                ///< #FF69B4
    IndianRed,              ///< #CD5C5C
    Indigo,                 ///< #4B0082
    Ivory,                  ///< #FFFFF0
    Khaki,                  ///< #F0E68C
    Lavender,               ///< #E6E6FA
    LavenderBlush,          ///< #FFF0F5
    LawnGreen,              ///< #7CFC00
    LemonChiffon,           ///< #FFFACD
    LightBlue,              ///< #ADD8E6
    LightCoral,             ///< #F08080
    LightCyan,              ///< #E0FFFF
    LightGoldenRodYellow,   ///< #FAFAD2
    LightGray,              ///< #D3D3D3
    LightGreen,             ///< #90EE90
    LightPink,              ///< #FFB6C1
    LightSalmon,            ///< #FFA07A
    LightSeaGreen,          ///< #20B2AA
    LightSkyBlue,           ///< #87CEFA
    LightSlateGray,         ///< #778899
    LightSteelBlue,         ///< #B0C4DE
    LightYellow,            ///< #FFFFE0
    Lime,                   ///< #00FF00
    LimeGreen,              ///< #32CD32
    Linen,                  ///< #FAF0E6
    Magenta,                ///< #FF00FF
    Maroon,                 ///< #800000
    MediumAquaMarine,       ///< #66CDAA
    MediumBlue,             ///< #0000CD
    MediumOrchid,           ///< #BA55D3
    MediumPurple,           ///< #9370DB
    MediumSeaGreen,         ///< #3CB371
    MediumSlateBlue,        ///< #7B68EE
    MediumSpringGreen,      ///< #00FA9A
    MediumTurquoise,        ///< #48D1CC
    MediumVioletRed,        ///< #C71585
    MidnightBlue,           ///< #191970
    MintCream,              ///< #F5FFFA
    MistyRose,              ///< #FFE4E1
    Moccasin,               ///< #FFE4B5
    NavajoWhite,            ///< #FFDEAD
    Navy,                   ///< #000080
    OldLace,                ///< #FDF5E6
    Olive,                  ///< #808000
    OliveDrab,              ///< #6B8E23
    Orange,                 ///< #FFA500
    OrangeRed,              ///< #FF4500
    Orchid,                 ///< #DA70D6
    PaleGoldenRod,          ///< #EEE8AA
    PaleGreen,              ///< #98FB98
    PaleTurquoise,          ///< #AFEEEE
    PaleVioletRed,          ///< #DB7093
    PapayaWhip,             ///< #FFEFD5
    PeachPuff,              ///< #FFDAB9
    Peru,                   ///< #CD853F
    Pink,                   ///< #FFC0CB
    Plum,                   ///< #DDA0DD
    PowderBlue,             ///< #B0E0E6
    Purple,                 ///< #800080
    RebeccaPurple,          ///< #663399
    Red,                    ///< #FF0000
    RosyBrown,              ///< #BC8F8F
    RoyalBlue,              ///< #4169E1
    SaddleBrown,            ///< #8B4513
    Salmon,                 ///< #FA8072
    SandyBrown,             ///< #F4A460
    SeaGreen,               ///< #2E8B57
    SeaShell,               ///< #FFF5EE
    Sienna,                 ///< #A0522D
    Silver,                 ///< #C0C0C0
    SkyBlue,                ///< #87CEEB
    SlateBlue,              ///< #6A5ACD
    SlateGray,              ///< #708090
    Snow,                   ///< #FFFAFA
    SpringGreen,            ///< #00FF7F
    SteelBlue,              ///< #4682B4
    Tan,                    ///< #D2B48C
    Teal,                   ///< #008080
    Thistle,                ///< #D8BFD8
    Tomato,                 ///< #FF6347
    Turquoise,              ///< #40E0D0
    Violet,                 ///< #EE82EE
    Wheat,                  ///< #F5DEB3
    White,                  ///< #FFFFFF
    WhiteSmoke,             ///< #F5F5F5
    Yellow,                 ///< #FFFF00
    YellowGreen,            ///< #9ACD32

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

