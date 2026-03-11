#pragma once

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u()
{
#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  m_v.Set(0xCDCDCDCD);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezUInt32 xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezInternal::QuadUInt v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE void ezSimdVec4u::Set(ezUInt32 xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4u::Set(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4u::SetZero()
{
  m_v.SetZero();
}

// needs to be implemented here because of include dependencies
EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(const ezSimdVec4u& u)
  : m_v(u.m_v.x, u.m_v.y, u.m_v.z, u.m_v.w)
{
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(const ezSimdVec4i& i)
  : m_v(i.m_v.x, i.m_v.y, i.m_v.z, i.m_v.w)
{
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4u::ToFloat() const
{
  ezSimdVec4f result;
  result.m_v.x = (float)m_v.x;
  result.m_v.y = (float)m_v.y;
  result.m_v.z = (float)m_v.z;
  result.m_v.w = (float)m_v.w;

  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::Truncate(const ezSimdVec4f& f)
{
  ezSimdVec4f clampedF = f.CompMax(ezSimdVec4f::MakeZero());

  ezSimdVec4u result;
  result.m_v.x = (ezUInt32)clampedF.m_v.x;
  result.m_v.y = (ezUInt32)clampedF.m_v.y;
  result.m_v.z = (ezUInt32)clampedF.m_v.z;
  result.m_v.w = (ezUInt32)clampedF.m_v.w;

  return result;
}

template <int N>
EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::GetComponent() const
{
  if constexpr (N == 0)
  {
    return m_v.x;
  }
  else if constexpr (N == 1)
  {
    return m_v.y;
  }
  else if constexpr (N == 2)
  {
    return m_v.z;
  }
  else if constexpr (N == 3)
  {
    return m_v.w;
  }
  else
  {
    return m_v.w;
  }
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::x() const
{
  return m_v.x;
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::y() const
{
  return m_v.y;
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::z() const
{
  return m_v.z;
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::w() const
{
  return m_v.w;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::Get() const
{
  ezSimdVec4u result;

  const ezUInt32* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator+(const ezSimdVec4u& v) const
{
  return m_v + v.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator-(const ezSimdVec4u& v) const
{
  return m_v - v.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMul(const ezSimdVec4u& v) const
{
  return m_v.CompMul(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator|(const ezSimdVec4u& v) const
{
  ezSimdVec4u result;
  result.m_v.x = m_v.x | v.m_v.x;
  result.m_v.y = m_v.y | v.m_v.y;
  result.m_v.z = m_v.z | v.m_v.z;
  result.m_v.w = m_v.w | v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator&(const ezSimdVec4u& v) const
{
  ezSimdVec4u result;
  result.m_v.x = m_v.x & v.m_v.x;
  result.m_v.y = m_v.y & v.m_v.y;
  result.m_v.z = m_v.z & v.m_v.z;
  result.m_v.w = m_v.w & v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator^(const ezSimdVec4u& v) const
{
  ezSimdVec4u result;
  result.m_v.x = m_v.x ^ v.m_v.x;
  result.m_v.y = m_v.y ^ v.m_v.y;
  result.m_v.z = m_v.z ^ v.m_v.z;
  result.m_v.w = m_v.w ^ v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator~() const
{
  ezSimdVec4u result;
  result.m_v.x = ~m_v.x;
  result.m_v.y = ~m_v.y;
  result.m_v.z = ~m_v.z;
  result.m_v.w = ~m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator<<(ezUInt32 uiShift) const
{
  ezSimdVec4u result;
  result.m_v.x = m_v.x << uiShift;
  result.m_v.y = m_v.y << uiShift;
  result.m_v.z = m_v.z << uiShift;
  result.m_v.w = m_v.w << uiShift;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator>>(ezUInt32 uiShift) const
{
  ezSimdVec4u result;
  result.m_v.x = m_v.x >> uiShift;
  result.m_v.y = m_v.y >> uiShift;
  result.m_v.z = m_v.z >> uiShift;
  result.m_v.w = m_v.w >> uiShift;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator+=(const ezSimdVec4u& v)
{
  m_v += v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator-=(const ezSimdVec4u& v)
{
  m_v -= v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator|=(const ezSimdVec4u& v)
{
  m_v.x |= v.m_v.x;
  m_v.y |= v.m_v.y;
  m_v.z |= v.m_v.z;
  m_v.w |= v.m_v.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator&=(const ezSimdVec4u& v)
{
  m_v.x &= v.m_v.x;
  m_v.y &= v.m_v.y;
  m_v.z &= v.m_v.z;
  m_v.w &= v.m_v.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator^=(const ezSimdVec4u& v)
{
  m_v.x ^= v.m_v.x;
  m_v.y ^= v.m_v.y;
  m_v.z ^= v.m_v.z;
  m_v.w ^= v.m_v.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator<<=(ezUInt32 uiShift)
{
  m_v.x <<= uiShift;
  m_v.y <<= uiShift;
  m_v.z <<= uiShift;
  m_v.w <<= uiShift;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator>>=(ezUInt32 uiShift)
{
  m_v.x >>= uiShift;
  m_v.y >>= uiShift;
  m_v.z >>= uiShift;
  m_v.w >>= uiShift;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMin(const ezSimdVec4u& v) const
{
  return m_v.CompMin(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMax(const ezSimdVec4u& v) const
{
  return m_v.CompMax(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator==(const ezSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x == v.m_v.x;
  result[1] = m_v.y == v.m_v.y;
  result[2] = m_v.z == v.m_v.z;
  result[3] = m_v.w == v.m_v.w;

  return ezSimdVec4b(result[0], result[1], result[2], result[3]);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator!=(const ezSimdVec4u& v) const
{
  return !(*this == v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator<=(const ezSimdVec4u& v) const
{
  return !(*this > v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator<(const ezSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x < v.m_v.x;
  result[1] = m_v.y < v.m_v.y;
  result[2] = m_v.z < v.m_v.z;
  result[3] = m_v.w < v.m_v.w;

  return ezSimdVec4b(result[0], result[1], result[2], result[3]);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator>=(const ezSimdVec4u& v) const
{
  return !(*this < v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator>(const ezSimdVec4u& v) const
{
  bool result[4];
  result[0] = m_v.x > v.m_v.x;
  result[1] = m_v.y > v.m_v.y;
  result[2] = m_v.z > v.m_v.z;
  result[3] = m_v.w > v.m_v.w;

  return ezSimdVec4b(result[0], result[1], result[2], result[3]);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::MakeZero()
{
  return ezVec4U32::MakeZero();
}
