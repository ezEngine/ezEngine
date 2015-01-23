#include <Foundation/PCH.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Color8UNorm.h>

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

