#pragma once

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f()
{
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(float xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(const ezSimdFloat& xyzw)
{
  m_v = xyzw.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(ezInternal::QuadFloat v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetZero()
{
  m_v.SetZero();
}

template<int N>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load(const float* pFloats)
{
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pFloats[i];
  }
}

template<int N>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store(float* pFloats) const
{
  for (int i = 0; i < N; ++i)
  {
    pFloats[i] = (&m_v.x)[i];
  }
}



template<int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsNaN() const
{
  for (int i = 0; i < N; ++i)
  {
    if (ezMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}



EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::x() const
{
  return m_v.x;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::y() const
{
  return m_v.y;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::z() const
{
  return m_v.z;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::w() const
{
  return m_v.w;
}



//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::ZeroVector()
{
  return ezVec4::ZeroVector();
}


#if 0
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator+(const ezSimdVec4f& f) const
{
  ezSimdVec4f result;
  result.x = m_impl.x + f.m_impl.x;
  result.y = m_impl.y + f.m_impl.y;
  result.z = m_impl.z + f.m_impl.z;
  result.w = m_impl.w + f.m_impl.w;
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator-(const ezSimdVec4f& f) const
{
  ezSimdVec4f result;
  result.x = m_impl.x - f.m_impl.x;
  result.y = m_impl.y - f.m_impl.y;
  result.z = m_impl.z - f.m_impl.z;
  result.w = m_impl.w - f.m_impl.w;
  return result;
}


EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator*(const ezSimdFloat& f) const
{
  ezSimdVec4f result;
  result.x = m_impl.x * f.m_impl;
  result.y = m_impl.y * f.m_impl;
  result.z = m_impl.z * f.m_impl;
  result.w = m_impl.w * f.m_impl;
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f operator/(const ezSimdFloat& f) const
{
  ezSimdVec4f result;
  result.x = m_impl.x / f.m_impl;
  result.y = m_impl.y / f.m_impl;
  result.z = m_impl.z / f.m_impl;
  result.w = m_impl.w / f.m_impl;
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f& operator+=(const ezSimdVec4f& f)
{
  m_impl.x += f.m_impl.x;
  m_impl.y += f.m_impl.y;
  m_impl.z += f.m_impl.z;
  m_impl.w += f.m_impl.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& operator-=(const ezSimdVec4f& f)
{
  m_impl.x -= f.m_impl.x;
  m_impl.y -= f.m_impl.y;
  m_impl.z -= f.m_impl.z;
  m_impl.w -= f.m_impl.w;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& operator*=(const ezSimdFloat& f)
{
  m_impl.x *= f.m_impl;
  m_impl.y *= f.m_impl;
  m_impl.z *= f.m_impl;
  m_impl.w *= f.m_impl;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& operator/=(const ezSimdFloat& f)
{
  m_impl.x /= f.m_impl;
  m_impl.y /= f.m_impl;
  m_impl.z /= f.m_impl;
  m_impl.w /= f.m_impl;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::cross(const ezSimdFloat& f) const
{
  ezSimdVec4f result;
  result.m_impl.x = m_impl.y * f.m_impl.z - m_impl.z * f.m_impl.y;
  result.m_impl.y = m_impl.z * f.m_impl.x - m_impl.x * f.m_impl.z;
  result.m_impl.z = m_impl.x * f.m_impl.y - m_impl.y * f.m_impl.x;
  return result;
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::dot<1>(const ezSimdVec4f& f) const
{
  return ezSimdFloat::FromImpl(m_impl.x * f.m_impl.x);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::dot<2>(const ezSimdVec4f& f) const
{
  return ezSimdFloat::FromImpl(m_impl.x * f.m_impl.x + m_impl.y * f.m_impl.y);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::dot<3>(const ezSimdVec4f& f) const
{
  return ezSimdFloat::FromImpl(m_impl.x * f.m_impl.x + m_impl.y * f.m_impl.y + m_impl.z * f.m_impl.z);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::dot<4>(const ezSimdVec4f& f) const
{
  return ezSimdFloat::FromImpl(m_impl.x * f.m_impl.x + m_impl.y * f.m_impl.y + m_impl.z * f.m_impl.z + m_impl.w * f.m_impl.w);
}
#endif
