#pragma once

EZ_FORCE_INLINE ezColor8UNorm::ezColor8UNorm()
{
}

EZ_FORCE_INLINE ezColor8UNorm::ezColor8UNorm(ezUInt8 R, ezUInt8 G, ezUInt8 B, ezUInt8 A /* = 255*/) :
  r(R), g(G), b(B), a(A)
{
}

inline ezColor8UNorm::ezColor8UNorm(const ezColor& color) :
  r(static_cast<ezUInt8>(color.r*255)),
  g(static_cast<ezUInt8>(color.g*255)),
  b(static_cast<ezUInt8>(color.b*255)),
  a(static_cast<ezUInt8>(color.a*255))
{
}

// *****************

inline ezColor8UNorm::operator ezColor () const
{
  return ezColor(r * (1.0f / 255.0f), g * (1.0f / 255.0f), b * (1.0f / 255.0f), a * (1.0f / 255.0f));
}

inline bool ezColor8UNorm::IsIdentical(const ezColor8UNorm& rhs) const
{
  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

// *****************

inline bool operator== (const ezColor8UNorm& c1, const ezColor8UNorm& c2)
{
  return c1.IsIdentical(c2);
}

inline bool operator!= (const ezColor8UNorm& c1, const ezColor8UNorm& c2)
{
  return !c1.IsIdentical(c2);
}







EZ_FORCE_INLINE ezColorBgra8UNorm::ezColorBgra8UNorm()
{
}

EZ_FORCE_INLINE ezColorBgra8UNorm::ezColorBgra8UNorm(ezUInt8 B, ezUInt8 G, ezUInt8 R, ezUInt8 A /* = 255*/) :
r(R), g(G), b(B), a(A)
{
}

inline ezColorBgra8UNorm::ezColorBgra8UNorm(const ezColor& color) :
r(static_cast<ezUInt8>(color.r * 255)),
g(static_cast<ezUInt8>(color.g * 255)),
b(static_cast<ezUInt8>(color.b * 255)),
a(static_cast<ezUInt8>(color.a * 255))
{
}

// *****************

inline ezColorBgra8UNorm::operator ezColor () const
{
  return ezColor(r * (1.0f / 255.0f), g * (1.0f / 255.0f), b * (1.0f / 255.0f), a * (1.0f / 255.0f));
}

inline bool ezColorBgra8UNorm::IsIdentical(const ezColorBgra8UNorm& rhs) const
{
  return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
}

// *****************

inline bool operator== (const ezColorBgra8UNorm& c1, const ezColorBgra8UNorm& c2)
{
  return c1.IsIdentical(c2);
}

inline bool operator!= (const ezColorBgra8UNorm& c1, const ezColorBgra8UNorm& c2)
{
  return !c1.IsIdentical(c2);
}

