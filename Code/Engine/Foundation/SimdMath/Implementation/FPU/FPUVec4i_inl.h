#pragma once

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_v.Set(0xCDCDCDCD);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInternal::QuadInt v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::SetZero()
{
  m_v.SetZero();
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4i::ToFloat() const
{
  ezSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Truncate(const ezSimdVec4f& f)
{
  ezSimdVec4i result;
  result.m_v.x = (ezInt32)f.m_v.x;
  result.m_v.y = (ezInt32)f.m_v.y;
  result.m_v.z = (ezInt32)f.m_v.z;
  result.m_v.w = (ezInt32)f.m_v.w;

  return result;
}

template <int N>
EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::GetComponent() const
{
  return (&m_v.x)[N];
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::x() const
{
  return m_v.x;
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::y() const
{
  return m_v.y;
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::z() const
{
  return m_v.z;
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::w() const
{
  return m_v.w;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Get() const
{
  ezSimdVec4i result;

  const ezInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator-() const
{
  return -m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator+(const ezSimdVec4i& v) const
{
  return m_v + v.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator-(const ezSimdVec4i& v) const
{
  return m_v - v.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMul(const ezSimdVec4i& v) const
{
  return m_v.CompMul(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator|(const ezSimdVec4i& v) const
{
  ezSimdVec4i result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator&(const ezSimdVec4i& v) const
{
  ezSimdVec4i result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator^(const ezSimdVec4i& v) const
{
  ezSimdVec4i result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator~() const
{
  ezSimdVec4i result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator<<(ezUInt32 uiShift) const
{
  ezSimdVec4i result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator>>(ezUInt32 uiShift) const
{
  ezSimdVec4i result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator+=(const ezSimdVec4i& v)
{
  m_v += v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator-=(const ezSimdVec4i& v)
{
  m_v -= v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator|=(const ezSimdVec4i& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator&=(const ezSimdVec4i& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator^=(const ezSimdVec4i& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator<<=(ezUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator>>=(ezUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMin(const ezSimdVec4i& v) const
{
  return m_v.CompMin(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMax(const ezSimdVec4i& v) const
{
  return m_v.CompMax(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Abs() const
{
  ezSimdVec4i result;
  result.m_v.x = ezMath::Abs(m_v.x);
  result.m_v.y = ezMath::Abs(m_v.y);
  result.m_v.z = ezMath::Abs(m_v.z);
  result.m_v.w = ezMath::Abs(m_v.w);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator==(const ezSimdVec4i& v) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x == v.m_v.x;
  result.m_v.y = m_v.y == v.m_v.y;
  result.m_v.z = m_v.z == v.m_v.z;
  result.m_v.w = m_v.w == v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator!=(const ezSimdVec4i& v) const
{
  return !(*this == v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator<=(const ezSimdVec4i& v) const
{
  return !(*this > v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator<(const ezSimdVec4i& v) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x < v.m_v.x;
  result.m_v.y = m_v.y < v.m_v.y;
  result.m_v.z = m_v.z < v.m_v.z;
  result.m_v.w = m_v.w < v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator>=(const ezSimdVec4i& v) const
{
  return !(*this < v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator>(const ezSimdVec4i& v) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x > v.m_v.x;
  result.m_v.y = m_v.y > v.m_v.y;
  result.m_v.z = m_v.z > v.m_v.z;
  result.m_v.w = m_v.w > v.m_v.w;

  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::ZeroVector()
{
  return ezVec4I32::ZeroVector();
}
