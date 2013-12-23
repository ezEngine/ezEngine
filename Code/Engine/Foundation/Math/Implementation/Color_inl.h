#pragma once

EZ_FORCE_INLINE ezColor::ezColor()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const float TypeNaN = ezMath::BasicType<float>::GetNaN();
  r = TypeNaN;
  g = TypeNaN;
  b = TypeNaN;
  a = TypeNaN;
#endif
}

EZ_FORCE_INLINE ezColor::ezColor(float r, float g, float b, float a /* = 1.0f */) : r(r), g(g), b(b), a(a)
{
}

template<typename Type>
EZ_FORCE_INLINE ezColor::ezColor(const ezVec4Template<Type>& v) :
   r(static_cast<float>(v.x)), g(static_cast<float>(v.y)), b(static_cast<float>(v.z)), a(static_cast<float>(v.w))
{}

// *****************

template<typename Type>
EZ_FORCE_INLINE ezColor::operator ezVec4Template<Type> () const
{
  return ezVec4Template<Type>(static_cast<Type>(r), static_cast<Type>(g), static_cast<Type>(b), static_cast<Type>(a));
}

template<typename Type>
EZ_FORCE_INLINE ezVec3Template<Type> ezColor::GetRGB() const
{
  return ezVec3Template<Type>(static_cast<Type>(r), static_cast<Type>(g), static_cast<Type>(b));
}

template<typename Type>
EZ_FORCE_INLINE ezVec3Template<Type> ezColor::GetBGR() const
{
  return ezVec3Template<Type>(static_cast<Type>(b), static_cast<Type>(g), static_cast<Type>(r));
}

template<typename Type>
EZ_FORCE_INLINE void ezColor::SetRGB(const ezVec3Template<Type>& rgb)
{
  r = static_cast<float>(rgb.x);
  g = static_cast<float>(rgb.y);
  b = static_cast<float>(rgb.z);
}

template<typename Type>
EZ_FORCE_INLINE void ezColor::SetBGR(const ezVec3Template<Type>& bgr)
{
  r = static_cast<float>(bgr.z);
  g = static_cast<float>(bgr.y);
  b = static_cast<float>(bgr.x);
}

// *****************

EZ_FORCE_INLINE bool ezColor::IsNormalized() const
{
  return r <= 1.0f && g <= 1.0f && b <= 1.0f && a <= 1.0f &&
    r >= 0.0f && g >= 0.0f && b >= 0.0f && a >= 0.0f; 
}

// http://en.literateprograms.org/RGB_to_HSV_color_space_conversion_%28C%29
template<typename Type>
inline ezVec3Template<Type> ezColor::ConvertToHSV() const
{
  ezVec3Template<Type> hsv;
  hsv.z = static_cast<Type>(ezMath::Max(r, g, b)); // Value
  if(hsv.z < ezMath::BasicType<Type>::SmallEpsilon())
  {
    hsv.y = hsv.x = 0.0f;
    return hsv;
  } 
  Type invV = static_cast<Type>(1.0) / hsv.z;
  Type norm_r = r * invV;
  Type norm_g = g * invV;
  Type norm_b = b * invV;
  Type rgb_min = ezMath::Min(norm_r, norm_g, norm_b);
  Type rgb_max = ezMath::Max(norm_r, norm_g, norm_b);

  hsv.y = rgb_max - rgb_min;  // Saturation
  if (hsv.y == 0)
  {
    hsv.x = 0;
    return hsv;
  }

  // Normalize saturation
  Type rgb_delta_inv = static_cast<Type>(1.0) / (rgb_max - rgb_min);
  norm_r = (norm_r - rgb_min) * rgb_delta_inv;
  norm_g = (norm_g - rgb_min) * rgb_delta_inv;
  norm_b = (norm_b - rgb_min) * rgb_delta_inv;
  rgb_min = ezMath::Min(norm_r, norm_g, norm_b);
  rgb_max = ezMath::Max(norm_r, norm_g, norm_b);

  // hue
  if(rgb_max == norm_r)
  {
    hsv.x = static_cast<Type>(60.0) * (norm_g - norm_b);
    if(hsv.x < 0.0)
      hsv.x += static_cast<Type>(360.0);
  }
  else if (rgb_max == norm_g)
    hsv.x = static_cast<Type>(120.0) + static_cast<Type>(60.0) * (norm_b - norm_r);
  else
    hsv.x = static_cast<Type>(240.0) + static_cast<Type>(60.0) * (norm_r - norm_g);

  return hsv;
}

// http://www.rapidtables.com/convert/color/hsv-to-rgb.htm
template<typename Type>
inline ezColor ezColor::FromHSV(Type hue, Type sat, Type val)
{
  EZ_ASSERT(hue <= 360 && hue >= 0, "HSV Hue value is in invalid range.");
  EZ_ASSERT(sat <= 1 && val >= 0, "HSV saturation value is in invalid range.");
  EZ_ASSERT(val <= 1 && val >= 0, "HSV value is in invalid range.");

  float c = static_cast<float>(sat * val);
  float x = c * (1.0f - ezMath::Abs(ezMath::Mod(hue / 60.0f, 2) - 1.0f));
  float m = static_cast<float>(val - c);
  
  if(hue < 60)
    return ezColor(c + m, x + m, 0 + m);
  else if(hue < 120)
    return ezColor(x + m, c + m, 0 + m);
  else if(hue < 180)
    return ezColor(0 + m, c + m, x + m);
  else if(hue < 240)
    return ezColor(0 + m, x + m, c + m);
  else if(hue < 300)
    return ezColor(x + m, 0 + m, c + m);
  else
    return ezColor(c + m, 0 + m, x + m);
}

inline float ezColor::GetSaturation() const
{
  return ConvertToHSV<float>().y;
}


// http://en.wikipedia.org/wiki/Luminance_%28relative%29
EZ_FORCE_INLINE float ezColor::GetLuminance() const
{
  return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

EZ_FORCE_INLINE ezColor ezColor::GetInvertedColor() const
{
  return ezColor(1.0f - r, 1.0f - g, 1.0f - b, 1.0f - a);
}

// http://en.wikipedia.org/wiki/SRGB
inline ezColor ezColor::ConvertLinearToSRGB() const
{
  // assuming we have linear color (not CIE xyY or CIE XYZ)
  return ezColor(r <= 0.0031308f ? (12.92f * r) : (1.055f * ezMath::Pow(r, 1.0f / 2.4f) - 0.055f),
    g <= 0.0031308f ? (12.92f * g) : (1.055f * ezMath::Pow(g, 1.0f / 2.4f) - 0.055f),
    b <= 0.0031308f ? (12.92f * b) : (1.055f * ezMath::Pow(b, 1.0f / 2.4f) - 0.055f),
    a);
}

inline ezColor ezColor::ConvertSRGBToLinear() const
{
  return ezColor(r <= 0.04045f ? (r / 12.92f) : (ezMath::Pow((r + 0.055f) / 1.055f, 2.4f)),
    g <= 0.04045f ? (g / 12.92f) : (ezMath::Pow((g + 0.055f) / 1.055f, 2.4f)),
    b <= 0.04045f ? (b / 12.92f) : (ezMath::Pow((b + 0.055f) / 1.055f, 2.4f)),
    a);
}

// *****************

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

inline bool ezColor::IsValid() const
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

// *****************

EZ_FORCE_INLINE void ezColor::operator+= (const ezColor& cc)
{
  r += cc.r;
  g += cc.g;
  b += cc.b;
  a += cc.a;
}

EZ_FORCE_INLINE void ezColor::operator-= (const ezColor& cc)
{
  r -= cc.r;
  g -= cc.g;
  b -= cc.b;
  a -= cc.a;
}

EZ_FORCE_INLINE void ezColor::operator*= (float f)
{
  r *= f;
  g *= f;
  b *= f;
  a *= f;
}

EZ_FORCE_INLINE void ezColor::operator/= (float f)
{
  float f_inv = 1.0f / f;
  r *= f_inv;
  g *= f_inv;
  b *= f_inv;
  a *= f_inv;
}

EZ_FORCE_INLINE bool ezColor::IsIdentical(const ezColor& rhs) const
{
  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

inline bool ezColor::IsEqual(const ezColor& rhs, float fEpsilon) const
{
  return (ezMath::IsEqual(r, rhs.r, fEpsilon) && 
          ezMath::IsEqual(g, rhs.g, fEpsilon) && 
          ezMath::IsEqual(b, rhs.b, fEpsilon) &&
          ezMath::IsEqual(a, rhs.a, fEpsilon));
}
// *****************

EZ_FORCE_INLINE const ezColor operator+ (const ezColor& c1, const ezColor& c2)
{
  return ezColor(c1.r + c2.r, c1.g + c2.g, c1.b + c2.b, c1.a + c2.a);
}

EZ_FORCE_INLINE const ezColor operator- (const ezColor& c1, const ezColor& c2)
{
  return ezColor(c1.r - c2.r, c1.g - c2.g, c1.b - c2.b, c1.a - c2.a);
}

template<typename Type>
EZ_FORCE_INLINE const ezColor operator* (Type f, const ezColor& c)
{
  return ezColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezColor operator* (const ezColor& c, Type f)
{
  return ezColor(c.r * f, c.g * f, c.b * f, c.a * f);
}

template<typename Type>
EZ_FORCE_INLINE const ezColor operator/ (const ezColor& c, Type f)
{
  float f_inv = 1.0f / f;
  return ezColor(c.r * f_inv, c.g * f_inv, c.b * f_inv, c.a * f_inv);
}

EZ_FORCE_INLINE bool operator== (const ezColor& c1, const ezColor& c2)
{
  return c1.IsIdentical(c2);
}

EZ_FORCE_INLINE bool operator!= (const ezColor& c1, const ezColor& c2)
{
  return !c1.IsIdentical(c2);
}



