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

bool ezColor::IsEqualRGB(const ezColor& rhs, float fEpsilon) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return (ezMath::IsEqual(r, rhs.r, fEpsilon) &&
          ezMath::IsEqual(g, rhs.g, fEpsilon) &&
          ezMath::IsEqual(b, rhs.b, fEpsilon));
}

bool ezColor::IsEqualRGBA(const ezColor& rhs, float fEpsilon) const
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

const ezColor ezColor::AliceBlue(ezColorGammaUB(0xF0, 0xF8, 0xFF));
const ezColor ezColor::AntiqueWhite(ezColorGammaUB(0xFA, 0xEB, 0xD7));
const ezColor ezColor::Aqua(ezColorGammaUB(0x00, 0xFF, 0xFF));
const ezColor ezColor::Aquamarine(ezColorGammaUB(0x7F, 0xFF, 0xD4));
const ezColor ezColor::Azure(ezColorGammaUB(0xF0, 0xFF, 0xFF));
const ezColor ezColor::Beige(ezColorGammaUB(0xF5, 0xF5, 0xDC));
const ezColor ezColor::Bisque(ezColorGammaUB(0xFF, 0xE4, 0xC4));
const ezColor ezColor::Black(ezColorGammaUB(0x00, 0x00, 0x00));
const ezColor ezColor::BlanchedAlmond(ezColorGammaUB(0xFF, 0xEB, 0xCD));
const ezColor ezColor::Blue(ezColorGammaUB(0x00, 0x00, 0xFF));
const ezColor ezColor::BlueViolet(ezColorGammaUB(0x8A, 0x2B, 0xE2));
const ezColor ezColor::Brown(ezColorGammaUB(0xA5, 0x2A, 0x2A));
const ezColor ezColor::BurlyWood(ezColorGammaUB(0xDE, 0xB8, 0x87));
const ezColor ezColor::CadetBlue(ezColorGammaUB(0x5F, 0x9E, 0xA0));
const ezColor ezColor::Chartreuse(ezColorGammaUB(0x7F, 0xFF, 0x00));
const ezColor ezColor::Chocolate(ezColorGammaUB(0xD2, 0x69, 0x1E));
const ezColor ezColor::Coral(ezColorGammaUB(0xFF, 0x7F, 0x50));
const ezColor ezColor::CornflowerBlue(ezColorGammaUB(0x64, 0x95, 0xED));
const ezColor ezColor::Cornsilk(ezColorGammaUB(0xFF, 0xF8, 0xDC));
const ezColor ezColor::Crimson(ezColorGammaUB(0xDC, 0x14, 0x3C));
const ezColor ezColor::Cyan(ezColorGammaUB(0x00, 0xFF, 0xFF));
const ezColor ezColor::DarkBlue(ezColorGammaUB(0x00, 0x00, 0x8B));
const ezColor ezColor::DarkCyan(ezColorGammaUB(0x00, 0x8B, 0x8B));
const ezColor ezColor::DarkGoldenRod(ezColorGammaUB(0xB8, 0x86, 0x0B));
const ezColor ezColor::DarkGray(ezColorGammaUB(0xA9, 0xA9, 0xA9));
const ezColor ezColor::DarkGreen(ezColorGammaUB(0x00, 0x64, 0x00));
const ezColor ezColor::DarkKhaki(ezColorGammaUB(0xBD, 0xB7, 0x6B));
const ezColor ezColor::DarkMagenta(ezColorGammaUB(0x8B, 0x00, 0x8B));
const ezColor ezColor::DarkOliveGreen(ezColorGammaUB(0x55, 0x6B, 0x2F));
const ezColor ezColor::DarkOrange(ezColorGammaUB(0xFF, 0x8C, 0x00));
const ezColor ezColor::DarkOrchid(ezColorGammaUB(0x99, 0x32, 0xCC));
const ezColor ezColor::DarkRed(ezColorGammaUB(0x8B, 0x00, 0x00));
const ezColor ezColor::DarkSalmon(ezColorGammaUB(0xE9, 0x96, 0x7A));
const ezColor ezColor::DarkSeaGreen(ezColorGammaUB(0x8F, 0xBC, 0x8F));
const ezColor ezColor::DarkSlateBlue(ezColorGammaUB(0x48, 0x3D, 0x8B));
const ezColor ezColor::DarkSlateGray(ezColorGammaUB(0x2F, 0x4F, 0x4F));
const ezColor ezColor::DarkTurquoise(ezColorGammaUB(0x00, 0xCE, 0xD1));
const ezColor ezColor::DarkViolet(ezColorGammaUB(0x94, 0x00, 0xD3));
const ezColor ezColor::DeepPink(ezColorGammaUB(0xFF, 0x14, 0x93));
const ezColor ezColor::DeepSkyBlue(ezColorGammaUB(0x00, 0xBF, 0xFF));
const ezColor ezColor::DimGray(ezColorGammaUB(0x69, 0x69, 0x69));
const ezColor ezColor::DodgerBlue(ezColorGammaUB(0x1E, 0x90, 0xFF));
const ezColor ezColor::FireBrick(ezColorGammaUB(0xB2, 0x22, 0x22));
const ezColor ezColor::FloralWhite(ezColorGammaUB(0xFF, 0xFA, 0xF0));
const ezColor ezColor::ForestGreen(ezColorGammaUB(0x22, 0x8B, 0x22));
const ezColor ezColor::Fuchsia(ezColorGammaUB(0xFF, 0x00, 0xFF));
const ezColor ezColor::Gainsboro(ezColorGammaUB(0xDC, 0xDC, 0xDC));
const ezColor ezColor::GhostWhite(ezColorGammaUB(0xF8, 0xF8, 0xFF));
const ezColor ezColor::Gold(ezColorGammaUB(0xFF, 0xD7, 0x00));
const ezColor ezColor::GoldenRod(ezColorGammaUB(0xDA, 0xA5, 0x20));
const ezColor ezColor::Gray(ezColorGammaUB(0x80, 0x80, 0x80));
const ezColor ezColor::Green(ezColorGammaUB(0x00, 0x80, 0x00));
const ezColor ezColor::GreenYellow(ezColorGammaUB(0xAD, 0xFF, 0x2F));
const ezColor ezColor::HoneyDew(ezColorGammaUB(0xF0, 0xFF, 0xF0));
const ezColor ezColor::HotPink(ezColorGammaUB(0xFF, 0x69, 0xB4));
const ezColor ezColor::IndianRed(ezColorGammaUB(0xCD, 0x5C, 0x5C));
const ezColor ezColor::Indigo(ezColorGammaUB(0x4B, 0x00, 0x82));
const ezColor ezColor::Ivory(ezColorGammaUB(0xFF, 0xFF, 0xF0));
const ezColor ezColor::Khaki(ezColorGammaUB(0xF0, 0xE6, 0x8C));
const ezColor ezColor::Lavender(ezColorGammaUB(0xE6, 0xE6, 0xFA));
const ezColor ezColor::LavenderBlush(ezColorGammaUB(0xFF, 0xF0, 0xF5));
const ezColor ezColor::LawnGreen(ezColorGammaUB(0x7C, 0xFC, 0x00));
const ezColor ezColor::LemonChiffon(ezColorGammaUB(0xFF, 0xFA, 0xCD));
const ezColor ezColor::LightBlue(ezColorGammaUB(0xAD, 0xD8, 0xE6));
const ezColor ezColor::LightCoral(ezColorGammaUB(0xF0, 0x80, 0x80));
const ezColor ezColor::LightCyan(ezColorGammaUB(0xE0, 0xFF, 0xFF));
const ezColor ezColor::LightGoldenRodYellow(ezColorGammaUB(0xFA, 0xFA, 0xD2));
const ezColor ezColor::LightGray(ezColorGammaUB(0xD3, 0xD3, 0xD3));
const ezColor ezColor::LightGreen(ezColorGammaUB(0x90, 0xEE, 0x90));
const ezColor ezColor::LightPink(ezColorGammaUB(0xFF, 0xB6, 0xC1));
const ezColor ezColor::LightSalmon(ezColorGammaUB(0xFF, 0xA0, 0x7A));
const ezColor ezColor::LightSeaGreen(ezColorGammaUB(0x20, 0xB2, 0xAA));
const ezColor ezColor::LightSkyBlue(ezColorGammaUB(0x87, 0xCE, 0xFA));
const ezColor ezColor::LightSlateGray(ezColorGammaUB(0x77, 0x88, 0x99));
const ezColor ezColor::LightSteelBlue(ezColorGammaUB(0xB0, 0xC4, 0xDE));
const ezColor ezColor::LightYellow(ezColorGammaUB(0xFF, 0xFF, 0xE0));
const ezColor ezColor::Lime(ezColorGammaUB(0x00, 0xFF, 0x00));
const ezColor ezColor::LimeGreen(ezColorGammaUB(0x32, 0xCD, 0x32));
const ezColor ezColor::Linen(ezColorGammaUB(0xFA, 0xF0, 0xE6));
const ezColor ezColor::Magenta(ezColorGammaUB(0xFF, 0x00, 0xFF));
const ezColor ezColor::Maroon(ezColorGammaUB(0x80, 0x00, 0x00));
const ezColor ezColor::MediumAquaMarine(ezColorGammaUB(0x66, 0xCD, 0xAA));
const ezColor ezColor::MediumBlue(ezColorGammaUB(0x00, 0x00, 0xCD));
const ezColor ezColor::MediumOrchid(ezColorGammaUB(0xBA, 0x55, 0xD3));
const ezColor ezColor::MediumPurple(ezColorGammaUB(0x93, 0x70, 0xDB));
const ezColor ezColor::MediumSeaGreen(ezColorGammaUB(0x3C, 0xB3, 0x71));
const ezColor ezColor::MediumSlateBlue(ezColorGammaUB(0x7B, 0x68, 0xEE));
const ezColor ezColor::MediumSpringGreen(ezColorGammaUB(0x00, 0xFA, 0x9A));
const ezColor ezColor::MediumTurquoise(ezColorGammaUB(0x48, 0xD1, 0xCC));
const ezColor ezColor::MediumVioletRed(ezColorGammaUB(0xC7, 0x15, 0x85));
const ezColor ezColor::MidnightBlue(ezColorGammaUB(0x19, 0x19, 0x70));
const ezColor ezColor::MintCream(ezColorGammaUB(0xF5, 0xFF, 0xFA));
const ezColor ezColor::MistyRose(ezColorGammaUB(0xFF, 0xE4, 0xE1));
const ezColor ezColor::Moccasin(ezColorGammaUB(0xFF, 0xE4, 0xB5));
const ezColor ezColor::NavajoWhite(ezColorGammaUB(0xFF, 0xDE, 0xAD));
const ezColor ezColor::Navy(ezColorGammaUB(0x00, 0x00, 0x80));
const ezColor ezColor::OldLace(ezColorGammaUB(0xFD, 0xF5, 0xE6));
const ezColor ezColor::Olive(ezColorGammaUB(0x80, 0x80, 0x00));
const ezColor ezColor::OliveDrab(ezColorGammaUB(0x6B, 0x8E, 0x23));
const ezColor ezColor::Orange(ezColorGammaUB(0xFF, 0xA5, 0x00));
const ezColor ezColor::OrangeRed(ezColorGammaUB(0xFF, 0x45, 0x00));
const ezColor ezColor::Orchid(ezColorGammaUB(0xDA, 0x70, 0xD6));
const ezColor ezColor::PaleGoldenRod(ezColorGammaUB(0xEE, 0xE8, 0xAA));
const ezColor ezColor::PaleGreen(ezColorGammaUB(0x98, 0xFB, 0x98));
const ezColor ezColor::PaleTurquoise(ezColorGammaUB(0xAF, 0xEE, 0xEE));
const ezColor ezColor::PaleVioletRed(ezColorGammaUB(0xDB, 0x70, 0x93));
const ezColor ezColor::PapayaWhip(ezColorGammaUB(0xFF, 0xEF, 0xD5));
const ezColor ezColor::PeachPuff(ezColorGammaUB(0xFF, 0xDA, 0xB9));
const ezColor ezColor::Peru(ezColorGammaUB(0xCD, 0x85, 0x3F));
const ezColor ezColor::Pink(ezColorGammaUB(0xFF, 0xC0, 0xCB));
const ezColor ezColor::Plum(ezColorGammaUB(0xDD, 0xA0, 0xDD));
const ezColor ezColor::PowderBlue(ezColorGammaUB(0xB0, 0xE0, 0xE6));
const ezColor ezColor::Purple(ezColorGammaUB(0x80, 0x00, 0x80));
const ezColor ezColor::RebeccaPurple(ezColorGammaUB(0x66, 0x33, 0x99));
const ezColor ezColor::Red(ezColorGammaUB(0xFF, 0x00, 0x00));
const ezColor ezColor::RosyBrown(ezColorGammaUB(0xBC, 0x8F, 0x8F));
const ezColor ezColor::RoyalBlue(ezColorGammaUB(0x41, 0x69, 0xE1));
const ezColor ezColor::SaddleBrown(ezColorGammaUB(0x8B, 0x45, 0x13));
const ezColor ezColor::Salmon(ezColorGammaUB(0xFA, 0x80, 0x72));
const ezColor ezColor::SandyBrown(ezColorGammaUB(0xF4, 0xA4, 0x60));
const ezColor ezColor::SeaGreen(ezColorGammaUB(0x2E, 0x8B, 0x57));
const ezColor ezColor::SeaShell(ezColorGammaUB(0xFF, 0xF5, 0xEE));
const ezColor ezColor::Sienna(ezColorGammaUB(0xA0, 0x52, 0x2D));
const ezColor ezColor::Silver(ezColorGammaUB(0xC0, 0xC0, 0xC0));
const ezColor ezColor::SkyBlue(ezColorGammaUB(0x87, 0xCE, 0xEB));
const ezColor ezColor::SlateBlue(ezColorGammaUB(0x6A, 0x5A, 0xCD));
const ezColor ezColor::SlateGray(ezColorGammaUB(0x70, 0x80, 0x90));
const ezColor ezColor::Snow(ezColorGammaUB(0xFF, 0xFA, 0xFA));
const ezColor ezColor::SpringGreen(ezColorGammaUB(0x00, 0xFF, 0x7F));
const ezColor ezColor::SteelBlue(ezColorGammaUB(0x46, 0x82, 0xB4));
const ezColor ezColor::Tan(ezColorGammaUB(0xD2, 0xB4, 0x8C));
const ezColor ezColor::Teal(ezColorGammaUB(0x00, 0x80, 0x80));
const ezColor ezColor::Thistle(ezColorGammaUB(0xD8, 0xBF, 0xD8));
const ezColor ezColor::Tomato(ezColorGammaUB(0xFF, 0x63, 0x47));
const ezColor ezColor::Turquoise(ezColorGammaUB(0x40, 0xE0, 0xD0));
const ezColor ezColor::Violet(ezColorGammaUB(0xEE, 0x82, 0xEE));
const ezColor ezColor::Wheat(ezColorGammaUB(0xF5, 0xDE, 0xB3));
const ezColor ezColor::White(ezColorGammaUB(0xFF, 0xFF, 0xFF));
const ezColor ezColor::WhiteSmoke(ezColorGammaUB(0xF5, 0xF5, 0xF5));
const ezColor ezColor::Yellow(ezColorGammaUB(0xFF, 0xFF, 0x00));
const ezColor ezColor::YellowGreen(ezColorGammaUB(0x9A, 0xCD, 0x32));




