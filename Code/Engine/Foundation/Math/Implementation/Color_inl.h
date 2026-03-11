#pragma once

inline ezColor::ezColor()
{
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float TypeNaN = ezMath::NaN<float>();
  r = TypeNaN;
  g = TypeNaN;
  b = TypeNaN;
  a = TypeNaN;
#endif
}

EZ_FORCE_INLINE constexpr ezColor::ezColor(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
  : r(fLinearRed)
  , g(fLinearGreen)
  , b(fLinearBlue)
  , a(fLinearAlpha)
{
}

inline ezColor::ezColor(const ezColorLinearUB& cc)
{
  *this = cc;
}

inline ezColor::ezColor(const ezColorGammaUB& cc)
{
  *this = cc;
}

inline void ezColor::SetRGB(float fLinearRed, float fLinearGreen, float fLinearBlue)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
}

inline void ezColor::SetRGBA(float fLinearRed, float fLinearGreen, float fLinearBlue, float fLinearAlpha /* = 1.0f */)
{
  r = fLinearRed;
  g = fLinearGreen;
  b = fLinearBlue;
  a = fLinearAlpha;
}

inline ezColor ezColor::MakeFromKelvin(ezUInt32 uiKelvin)
{
  ezColor finalColor;
  float kelvin = ezMath::Clamp(uiKelvin, 1000u, 40000u) / 1000.0f;
  float kelvin2 = kelvin * kelvin;

  finalColor.r = kelvin < 6.570f ? 1.0f : ezMath::Saturate((1.35651f + 0.216422f * kelvin + 0.000633715f * kelvin2) / (-3.24223f + 0.918711f * kelvin));
  finalColor.g = kelvin < 6.570f ? ezMath::Saturate((-399.809f + 414.271f * kelvin + 111.543f * kelvin2) / (2779.24f + 164.143f * kelvin + 84.7356f * kelvin2)) : ezMath::Saturate((1370.38f + 734.616f * kelvin + 0.689955f * kelvin2) / (-4625.69f + 1699.87f * kelvin));
  finalColor.b = kelvin > 6.570f ? 1.0f : ezMath::Saturate((348.963f - 523.53f * kelvin + 183.62f * kelvin2) / (2848.82f - 214.52f * kelvin + 78.8614f * kelvin2));
  finalColor.a = 1.0f;

  return finalColor;
}

// http://en.wikipedia.org/wiki/Luminance_%28relative%29
EZ_FORCE_INLINE float ezColor::GetLuminance() const
{
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

inline ezColor ezColor::GetInvertedColor() const
{
  EZ_NAN_ASSERT(this);
  EZ_ASSERT_DEBUG(IsNormalized(), "Cannot invert a color that has values outside the [0; 1] range");

  return ezColor(1.0f - r, 1.0f - g, 1.0f - b, 1.0f - a);
}

inline bool ezColor::IsNaN() const
{
  if (ezMath::IsNaN(r))
    return true;
  if (ezMath::IsNaN(g))
    return true;
  if (ezMath::IsNaN(b))
    return true;
  if (ezMath::IsNaN(a))
    return true;

  return false;
}

inline void ezColor::operator+=(const ezColor& rhs)
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  r += rhs.r;
  g += rhs.g;
  b += rhs.b;
  a += rhs.a;
}

inline void ezColor::operator-=(const ezColor& rhs)
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  r -= rhs.r;
  g -= rhs.g;
  b -= rhs.b;
  a -= rhs.a;
}

inline void ezColor::operator*=(const ezColor& rhs)
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  r *= rhs.r;
  g *= rhs.g;
  b *= rhs.b;
  a *= rhs.a;
}
inline void ezColor::operator*=(float f)
{
  r *= f;
  g *= f;
  b *= f;
  a *= f;

  EZ_NAN_ASSERT(this);
}

inline bool ezColor::IsIdenticalRGB(const ezColor& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b;
}

inline bool ezColor::IsIdenticalRGBA(const ezColor& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

inline ezColor ezColor::WithAlpha(float fAlpha) const
{
  return ezColor(r, g, b, fAlpha);
}

inline const ezColor operator+(const ezColor& c1, const ezColor& c2)
{
  EZ_NAN_ASSERT(&c1);
  EZ_NAN_ASSERT(&c2);

  return ezColor(c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a);
}

inline const ezColor operator-(const ezColor& c1, const ezColor& c2)
{
  EZ_NAN_ASSERT(&c1);
  EZ_NAN_ASSERT(&c2);

  return ezColor(c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a);
}

inline const ezColor operator*(const ezColor& c1, const ezColor& c2)
{
  EZ_NAN_ASSERT(&c1);
  EZ_NAN_ASSERT(&c2);

  return ezColor(c1.r * c2.r, c1.g * c2.g, c1.b * c2.b, c1.a * c2.a);
}

inline const ezColor operator*(float f, const ezColor& c)
{
  EZ_NAN_ASSERT(&c);

  return ezColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const ezColor operator*(const ezColor& c, float f)
{
  EZ_NAN_ASSERT(&c);

  return ezColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

inline const ezColor operator*(const ezMat4& lhs, const ezColor& rhs)
{
  ezColor r = rhs;
  r *= lhs;
  return r;
}

inline const ezColor operator/(const ezColor& c, float f)
{
  EZ_NAN_ASSERT(&c);

  float f_inv = 1.0f / f;
  return ezColor(c.r * f_inv, c.g * f_inv, c.b * f_inv, c.a * f_inv);
}

EZ_ALWAYS_INLINE bool operator==(const ezColor& c1, const ezColor& c2)
{
  return c1.IsIdenticalRGBA(c2);
}

EZ_ALWAYS_INLINE bool operator!=(const ezColor& c1, const ezColor& c2)
{
  return !c1.IsIdenticalRGBA(c2);
}

EZ_FORCE_INLINE bool operator<(const ezColor& c1, const ezColor& c2)
{
  if (c1.r < c2.r)
    return true;
  if (c1.r > c2.r)
    return false;
  if (c1.g < c2.g)
    return true;
  if (c1.g > c2.g)
    return false;
  if (c1.b < c2.b)
    return true;
  if (c1.b > c2.b)
    return false;

  return (c1.a < c2.a);
}
