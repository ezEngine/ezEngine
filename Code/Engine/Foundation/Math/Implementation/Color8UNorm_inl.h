#pragma once

EZ_ALWAYS_INLINE ezColorBaseUB::ezColorBaseUB(ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a /* = 255*/)
{
  this->r = r;
  this->g = g;
  this->b = b;
  this->a = a;
}

EZ_ALWAYS_INLINE ezColorLinearUB::ezColorLinearUB(ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a /* = 255*/)
  : ezColorBaseUB(r, g, b, a)
{
}

inline ezColorLinearUB::ezColorLinearUB(const ezColor& color)
{
  *this = color;
}

inline void ezColorLinearUB::operator=(const ezColor& color)
{
  r = ezMath::ColorFloatToByte(color.r);
  g = ezMath::ColorFloatToByte(color.g);
  b = ezMath::ColorFloatToByte(color.b);
  a = ezMath::ColorFloatToByte(color.a);
}

inline ezColor ezColorLinearUB::ToLinearFloat() const
{
  return ezColor(ezMath::ColorByteToFloat(r), ezMath::ColorByteToFloat(g), ezMath::ColorByteToFloat(b), ezMath::ColorByteToFloat(a));
}

// *****************

EZ_ALWAYS_INLINE ezColorGammaUB::ezColorGammaUB(ezUInt8 r, ezUInt8 g, ezUInt8 b, ezUInt8 a)
  : ezColorBaseUB(r, g, b, a)
{
}

inline ezColorGammaUB::ezColorGammaUB(const ezColor& color)
{
  *this = color;
}

inline void ezColorGammaUB::operator=(const ezColor& color)
{
  const ezVec3 gamma = ezColor::LinearToGamma(ezVec3(color.r, color.g, color.b));

  r = ezMath::ColorFloatToByte(gamma.x);
  g = ezMath::ColorFloatToByte(gamma.y);
  b = ezMath::ColorFloatToByte(gamma.z);
  a = ezMath::ColorFloatToByte(color.a);
}

inline ezColor ezColorGammaUB::ToLinearFloat() const
{
  ezVec3 gamma;
  gamma.x = ezMath::ColorByteToFloat(r);
  gamma.y = ezMath::ColorByteToFloat(g);
  gamma.z = ezMath::ColorByteToFloat(b);

  const ezVec3 linear = ezColor::GammaToLinear(gamma);

  return ezColor(linear.x, linear.y, linear.z, ezMath::ColorByteToFloat(a));
}
