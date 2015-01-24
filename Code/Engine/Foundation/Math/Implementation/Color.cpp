#include <Foundation/PCH.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Mat4.h>

// ****** ezColor ******

void ezColor::operator= (const ezColorLinearUB& cc)
{
  *this = cc.ToLinearFloat();
}

void ezColor::operator= (const ezColorGammaUB& cc)
{
  *this = cc.ToLinearFloat();
}

bool ezColor::IsNormalized() const
{
  EZ_NAN_ASSERT(this);

  return r <= 1.0f && g <= 1.0f && b <= 1.0f && a <= 1.0f &&
    r >= 0.0f && g >= 0.0f && b >= 0.0f && a >= 0.0f;
}

// http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29
void ezColor::ToLinearHSV(float& out_hue, float& out_sat, float& out_value) const
{
  out_value = ezMath::Max(r, g, b); // Value

  if (out_value < ezMath::BasicType<float>::SmallEpsilon())
  {
    out_hue = 0.0f;
    out_sat = 0.0f;
    out_value = 0.0f;
    return;
  }

  const float invV = 1.0f / out_value;
  float norm_r = r * invV;
  float norm_g = g * invV;
  float norm_b = b * invV;
  float rgb_min = ezMath::Min(norm_r, norm_g, norm_b);
  float rgb_max = ezMath::Max(norm_r, norm_g, norm_b);

  out_sat = rgb_max - rgb_min;  // Saturation

  if (out_sat == 0)
  {
    out_hue = 0;
    return;
  }

  // Normalize saturation
  const float rgb_delta_inv = 1.0f / (rgb_max - rgb_min);
  norm_r = (norm_r - rgb_min) * rgb_delta_inv;
  norm_g = (norm_g - rgb_min) * rgb_delta_inv;
  norm_b = (norm_b - rgb_min) * rgb_delta_inv;
  rgb_min = ezMath::Min(norm_r, norm_g, norm_b);
  rgb_max = ezMath::Max(norm_r, norm_g, norm_b);

  // hue
  if (rgb_max == norm_r)
  {
    out_hue = 60.0f * (norm_g - norm_b);

    if (out_hue < 0.0f)
      out_hue += 360.0f;
  }
  else if (rgb_max == norm_g)
    out_hue = 120.0f + 60.0f * (norm_b - norm_r);
  else
    out_hue = 240.0f + 60.0f * (norm_r - norm_g);
}

// http://www.rapidtables.com/convert/color/hsv-to-rgb.htm
void ezColor::FromLinearHSV(float hue, float sat, float val)
{
  EZ_ASSERT_DEBUG(hue <= 360 && hue >= 0, "HSV Hue value is in invalid range.");
  EZ_ASSERT_DEBUG(sat <= 1 && val >= 0, "HSV saturation value is in invalid range.");
  EZ_ASSERT_DEBUG(val <= 1 && val >= 0, "HSV value is in invalid range.");

  float c = sat * val;
  float x = c * (1.0f - ezMath::Abs(ezMath::Mod(hue / 60.0f, 2) - 1.0f));
  float m = val - c;


  a = 1.0f;

  if (hue < 60)
  {
    r = c + m;
    g = x + m;
    b = 0 + m;
  }
  else if (hue < 120)
  {
    r = x + m;
    g = c + m;
    b = 0 + m;
  }
  else if (hue < 180)
  {
    r = 0 + m;
    g = c + m;
    b = x + m;
  }
  else if (hue < 240)
  {
    r = 0 + m;
    g = x + m;
    b = c + m;
  }
  else if (hue < 300)
  {
    r = x + m;
    g = 0 + m;
    b = c + m;
  }
  else
  {
    r = c + m;
    g = 0 + m;
    b = x + m;
  }
}

float ezColor::GetSaturation() const
{
  float hue, sat, val;
  ToLinearHSV(hue, sat, val);

  return sat;
}

bool ezColor::IsValid() const
{
  if (!ezMath::IsFinite(r))
    return false;
  if (!ezMath::IsFinite(g))
    return false;
  if (!ezMath::IsFinite(b))
    return false;
  if (!ezMath::IsFinite(a))
    return false;

  return true;
}

bool ezColor::IsEqual(const ezColor& rhs, float fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return (ezMath::IsEqual(r, rhs.r, fEpsilon) &&
          ezMath::IsEqual(g, rhs.g, fEpsilon) &&
          ezMath::IsEqual(b, rhs.b, fEpsilon) &&
          ezMath::IsEqual(a, rhs.a, fEpsilon));
}

void ezColor::operator/= (float f)
{
  float f_inv = 1.0f / f;
  r *= f_inv;
  g *= f_inv;
  b *= f_inv;
  a *= f_inv;

  EZ_NAN_ASSERT(this);
}

void ezColor::operator*= (const ezMat4& rhs)
{
  ezVec3 v(r, g, b);
  v = rhs.TransformPosition(v);

  r = v.x;
  g = v.y;
  b = v.z;
}

ezColor ezColor::GetComplementaryColor() const
{
  float hue, sat, val;
  ToLinearHSV(hue, sat, val);

  ezColor Shifted;
  Shifted.FromLinearHSV(ezMath::Mod(hue + 180.0f, 360.0f), sat, val);
  Shifted.a = a;

  return Shifted;
}

void ezColor::FromGammaHSV(float hue, float sat, float val)
{
  ezColor gamma;
  gamma.FromLinearHSV(hue, sat, val);

  const ezVec3 linear = GammaToLinear(ezVec3(gamma.r, gamma.g, gamma.b));

  r = linear.x;
  g = linear.y;
  b = linear.z;
  a = gamma.a;
}

void ezColor::ToGammaHSV(float& hue, float& sat, float& val) const
{
  const ezVec3 gamma = LinearToGamma(ezVec3(r, g, b));

  ezColor ColorGamma(gamma.x, gamma.y, gamma.z, a);
  ColorGamma.ToLinearHSV(hue, sat, val);
}

ezVec3 ezColor::GammaToLinear(const ezVec3& gamma)
{
  return ezVec3(gamma.x <= 0.04045f ? (gamma.x / 12.92f) : (ezMath::Pow((gamma.x + 0.055f) / 1.055f, 2.4f)),
                gamma.y <= 0.04045f ? (gamma.y / 12.92f) : (ezMath::Pow((gamma.y + 0.055f) / 1.055f, 2.4f)),
                gamma.z <= 0.04045f ? (gamma.z / 12.92f) : (ezMath::Pow((gamma.z + 0.055f) / 1.055f, 2.4f)));
}

ezVec3 ezColor::LinearToGamma(const ezVec3& linear)
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return ezVec3(linear.x <= 0.0031308f ? (12.92f * linear.x) : (1.055f * ezMath::Pow(linear.x, 1.0f / 2.4f) - 0.055f),
                linear.y <= 0.0031308f ? (12.92f * linear.y) : (1.055f * ezMath::Pow(linear.y, 1.0f / 2.4f) - 0.055f),
                linear.z <= 0.0031308f ? (12.92f * linear.z) : (1.055f * ezMath::Pow(linear.z, 1.0f / 2.4f) - 0.055f));
}

ezColor ezColor::s_PredefinedColors[(ezUInt32) Predefined::ENUM_COUNT] =
{
  ezColorGammaUB(0xF0, 0xF8, 0xFF),   // AliceBlue
  ezColorGammaUB(0xFA, 0xEB, 0xD7),    // AntiqueWhite
  ezColorGammaUB(0x00, 0xFF, 0xFF),    // Aqua
  ezColorGammaUB(0x7F, 0xFF, 0xD4),    // Aquamarine
  ezColorGammaUB(0xF0, 0xFF, 0xFF),    // Azure
  ezColorGammaUB(0xF5, 0xF5, 0xDC),    // Beige
  ezColorGammaUB(0xFF, 0xE4, 0xC4),    // Bisque
  ezColorGammaUB(0x00, 0x00, 0x00),    // Black
  ezColorGammaUB(0xFF, 0xEB, 0xCD),    // BlanchedAlmond
  ezColorGammaUB(0x00, 0x00, 0xFF),    // Blue
  ezColorGammaUB(0x8A, 0x2B, 0xE2),    // BlueViolet
  ezColorGammaUB(0xA5, 0x2A, 0x2A),    // Brown
  ezColorGammaUB(0xDE, 0xB8, 0x87),    // BurlyWood
  ezColorGammaUB(0x5F, 0x9E, 0xA0),    // CadetBlue
  ezColorGammaUB(0x7F, 0xFF, 0x00),    // Chartreuse
  ezColorGammaUB(0xD2, 0x69, 0x1E),    // Chocolate
  ezColorGammaUB(0xFF, 0x7F, 0x50),    // Coral
  ezColorGammaUB(0x64, 0x95, 0xED),    // CornflowerBlue
  ezColorGammaUB(0xFF, 0xF8, 0xDC),    // Cornsilk
  ezColorGammaUB(0xDC, 0x14, 0x3C),    // Crimson
  ezColorGammaUB(0x00, 0xFF, 0xFF),    // Cyan
  ezColorGammaUB(0x00, 0x00, 0x8B),    // DarkBlue
  ezColorGammaUB(0x00, 0x8B, 0x8B),    // DarkCyan
  ezColorGammaUB(0xB8, 0x86, 0x0B),    // DarkGoldenRod
  ezColorGammaUB(0xA9, 0xA9, 0xA9),    // DarkGray
  ezColorGammaUB(0x00, 0x64, 0x00),    // DarkGreen
  ezColorGammaUB(0xBD, 0xB7, 0x6B),    // DarkKhaki
  ezColorGammaUB(0x8B, 0x00, 0x8B),    // DarkMagenta
  ezColorGammaUB(0x55, 0x6B, 0x2F),    // DarkOliveGreen
  ezColorGammaUB(0xFF, 0x8C, 0x00),    // DarkOrange
  ezColorGammaUB(0x99, 0x32, 0xCC),    // DarkOrchid
  ezColorGammaUB(0x8B, 0x00, 0x00),    // DarkRed
  ezColorGammaUB(0xE9, 0x96, 0x7A),    // DarkSalmon
  ezColorGammaUB(0x8F, 0xBC, 0x8F),    // DarkSeaGreen
  ezColorGammaUB(0x48, 0x3D, 0x8B),    // DarkSlateBlue
  ezColorGammaUB(0x2F, 0x4F, 0x4F),    // DarkSlateGray
  ezColorGammaUB(0x00, 0xCE, 0xD1),    // DarkTurquoise
  ezColorGammaUB(0x94, 0x00, 0xD3),    // DarkViolet
  ezColorGammaUB(0xFF, 0x14, 0x93),    // DeepPink
  ezColorGammaUB(0x00, 0xBF, 0xFF),    // DeepSkyBlue
  ezColorGammaUB(0x69, 0x69, 0x69),    // DimGray
  ezColorGammaUB(0x1E, 0x90, 0xFF),    // DodgerBlue
  ezColorGammaUB(0xB2, 0x22, 0x22),    // FireBrick
  ezColorGammaUB(0xFF, 0xFA, 0xF0),    // FloralWhite
  ezColorGammaUB(0x22, 0x8B, 0x22),    // ForestGreen
  ezColorGammaUB(0xFF, 0x00, 0xFF),    // Fuchsia
  ezColorGammaUB(0xDC, 0xDC, 0xDC),    // Gainsboro
  ezColorGammaUB(0xF8, 0xF8, 0xFF),    // GhostWhite
  ezColorGammaUB(0xFF, 0xD7, 0x00),    // Gold
  ezColorGammaUB(0xDA, 0xA5, 0x20),    // GoldenRod
  ezColorGammaUB(0x80, 0x80, 0x80),    // Gray
  ezColorGammaUB(0x00, 0x80, 0x00),    // Green
  ezColorGammaUB(0xAD, 0xFF, 0x2F),    // GreenYellow
  ezColorGammaUB(0xF0, 0xFF, 0xF0),    // HoneyDew
  ezColorGammaUB(0xFF, 0x69, 0xB4),    // HotPink
  ezColorGammaUB(0xCD, 0x5C, 0x5C),    // IndianRed
  ezColorGammaUB(0x4B, 0x00, 0x82),    // Indigo
  ezColorGammaUB(0xFF, 0xFF, 0xF0),    // Ivory
  ezColorGammaUB(0xF0, 0xE6, 0x8C),    // Khaki
  ezColorGammaUB(0xE6, 0xE6, 0xFA),    // Lavender
  ezColorGammaUB(0xFF, 0xF0, 0xF5),    // LavenderBlush
  ezColorGammaUB(0x7C, 0xFC, 0x00),    // LawnGreen
  ezColorGammaUB(0xFF, 0xFA, 0xCD),    // LemonChiffon
  ezColorGammaUB(0xAD, 0xD8, 0xE6),    // LightBlue
  ezColorGammaUB(0xF0, 0x80, 0x80),    // LightCoral
  ezColorGammaUB(0xE0, 0xFF, 0xFF),    // LightCyan
  ezColorGammaUB(0xFA, 0xFA, 0xD2),    // LightGoldenRodYellow
  ezColorGammaUB(0xD3, 0xD3, 0xD3),    // LightGray
  ezColorGammaUB(0x90, 0xEE, 0x90),    // LightGreen
  ezColorGammaUB(0xFF, 0xB6, 0xC1),    // LightPink
  ezColorGammaUB(0xFF, 0xA0, 0x7A),    // LightSalmon
  ezColorGammaUB(0x20, 0xB2, 0xAA),    // LightSeaGreen
  ezColorGammaUB(0x87, 0xCE, 0xFA),    // LightSkyBlue
  ezColorGammaUB(0x77, 0x88, 0x99),    // LightSlateGray
  ezColorGammaUB(0xB0, 0xC4, 0xDE),    // LightSteelBlue
  ezColorGammaUB(0xFF, 0xFF, 0xE0),    // LightYellow
  ezColorGammaUB(0x00, 0xFF, 0x00),    // Lime
  ezColorGammaUB(0x32, 0xCD, 0x32),    // LimeGreen
  ezColorGammaUB(0xFA, 0xF0, 0xE6),    // Linen
  ezColorGammaUB(0xFF, 0x00, 0xFF),    // Magenta
  ezColorGammaUB(0x80, 0x00, 0x00),    // Maroon
  ezColorGammaUB(0x66, 0xCD, 0xAA),    // MediumAquaMarine
  ezColorGammaUB(0x00, 0x00, 0xCD),    // MediumBlue
  ezColorGammaUB(0xBA, 0x55, 0xD3),    // MediumOrchid
  ezColorGammaUB(0x93, 0x70, 0xDB),    // MediumPurple
  ezColorGammaUB(0x3C, 0xB3, 0x71),    // MediumSeaGreen
  ezColorGammaUB(0x7B, 0x68, 0xEE),    // MediumSlateBlue
  ezColorGammaUB(0x00, 0xFA, 0x9A),    // MediumSpringGreen
  ezColorGammaUB(0x48, 0xD1, 0xCC),    // MediumTurquoise
  ezColorGammaUB(0xC7, 0x15, 0x85),    // MediumVioletRed
  ezColorGammaUB(0x19, 0x19, 0x70),    // MidnightBlue
  ezColorGammaUB(0xF5, 0xFF, 0xFA),    // MintCream
  ezColorGammaUB(0xFF, 0xE4, 0xE1),    // MistyRose
  ezColorGammaUB(0xFF, 0xE4, 0xB5),    // Moccasin
  ezColorGammaUB(0xFF, 0xDE, 0xAD),    // NavajoWhite
  ezColorGammaUB(0x00, 0x00, 0x80),    // Navy
  ezColorGammaUB(0xFD, 0xF5, 0xE6),    // OldLace
  ezColorGammaUB(0x80, 0x80, 0x00),    // Olive
  ezColorGammaUB(0x6B, 0x8E, 0x23),    // OliveDrab
  ezColorGammaUB(0xFF, 0xA5, 0x00),    // Orange
  ezColorGammaUB(0xFF, 0x45, 0x00),    // OrangeRed
  ezColorGammaUB(0xDA, 0x70, 0xD6),    // Orchid
  ezColorGammaUB(0xEE, 0xE8, 0xAA),    // PaleGoldenRod
  ezColorGammaUB(0x98, 0xFB, 0x98),    // PaleGreen
  ezColorGammaUB(0xAF, 0xEE, 0xEE),    // PaleTurquoise
  ezColorGammaUB(0xDB, 0x70, 0x93),    // PaleVioletRed
  ezColorGammaUB(0xFF, 0xEF, 0xD5),    // PapayaWhip
  ezColorGammaUB(0xFF, 0xDA, 0xB9),    // PeachPuff
  ezColorGammaUB(0xCD, 0x85, 0x3F),    // Peru
  ezColorGammaUB(0xFF, 0xC0, 0xCB),    // Pink
  ezColorGammaUB(0xDD, 0xA0, 0xDD),    // Plum
  ezColorGammaUB(0xB0, 0xE0, 0xE6),    // PowderBlue
  ezColorGammaUB(0x80, 0x00, 0x80),    // Purple
  ezColorGammaUB(0x66, 0x33, 0x99),    // RebeccaPurple
  ezColorGammaUB(0xFF, 0x00, 0x00),    // Red
  ezColorGammaUB(0xBC, 0x8F, 0x8F),    // RosyBrown
  ezColorGammaUB(0x41, 0x69, 0xE1),    // RoyalBlue
  ezColorGammaUB(0x8B, 0x45, 0x13),    // SaddleBrown
  ezColorGammaUB(0xFA, 0x80, 0x72),    // Salmon
  ezColorGammaUB(0xF4, 0xA4, 0x60),    // SandyBrown
  ezColorGammaUB(0x2E, 0x8B, 0x57),    // SeaGreen
  ezColorGammaUB(0xFF, 0xF5, 0xEE),    // SeaShell
  ezColorGammaUB(0xA0, 0x52, 0x2D),    // Sienna
  ezColorGammaUB(0xC0, 0xC0, 0xC0),    // Silver
  ezColorGammaUB(0x87, 0xCE, 0xEB),    // SkyBlue
  ezColorGammaUB(0x6A, 0x5A, 0xCD),    // SlateBlue
  ezColorGammaUB(0x70, 0x80, 0x90),    // SlateGray
  ezColorGammaUB(0xFF, 0xFA, 0xFA),    // Snow
  ezColorGammaUB(0x00, 0xFF, 0x7F),    // SpringGreen
  ezColorGammaUB(0x46, 0x82, 0xB4),    // SteelBlue
  ezColorGammaUB(0xD2, 0xB4, 0x8C),    // Tan
  ezColorGammaUB(0x00, 0x80, 0x80),    // Teal
  ezColorGammaUB(0xD8, 0xBF, 0xD8),    // Thistle
  ezColorGammaUB(0xFF, 0x63, 0x47),    // Tomato
  ezColorGammaUB(0x40, 0xE0, 0xD0),    // Turquoise
  ezColorGammaUB(0xEE, 0x82, 0xEE),    // Violet
  ezColorGammaUB(0xF5, 0xDE, 0xB3),    // Wheat
  ezColorGammaUB(0xFF, 0xFF, 0xFF),    // White
  ezColorGammaUB(0xF5, 0xF5, 0xF5),    // WhiteSmoke
  ezColorGammaUB(0xFF, 0xFF, 0x00),    // Yellow
  ezColorGammaUB(0x9A, 0xCD, 0x32),    // YellowGreen
};



