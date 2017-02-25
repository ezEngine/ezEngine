#pragma once

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat()
{
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(float f)
{
  m_v.Set(f);
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezInt32 i)
{
  m_v.Set((float)i);
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezUInt32 i)
{
  m_v.Set((float)i);
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezAngle a)
{
  m_v.Set(a.GetRadian());
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezInternal::QuadFloat v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdFloat::operator float() const
{
  return m_v.x;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator+(const ezSimdFloat& f) const
{
  return m_v + f.m_v;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator-(const ezSimdFloat& f) const
{
  return m_v - f.m_v;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator*(const ezSimdFloat& f) const
{
  return m_v.CompMult(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator/(const ezSimdFloat& f) const
{
  return m_v.CompDiv(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator+=(const ezSimdFloat& f)
{
  m_v += f.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator-=(const ezSimdFloat& f)
{
  m_v -= f.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator*=(const ezSimdFloat& f)
{
  m_v = m_v.CompMult(f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator/=(const ezSimdFloat& f)
{
  m_v = m_v.CompDiv(f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::IsEqual(const ezSimdFloat& rhs, const ezSimdFloat& fEpsilon) const
{
  return m_v.IsEqual(rhs.m_v, fEpsilon);
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator==(const ezSimdFloat& f) const
{
  return m_v.x == f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator!=(const ezSimdFloat& f) const
{
  return m_v.x != f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>=(const ezSimdFloat& f) const
{
  return m_v.x >= f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>(const ezSimdFloat& f) const
{
  return m_v.x > f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<=(const ezSimdFloat& f) const
{
  return m_v.x <= f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<(const ezSimdFloat& f) const
{
  return m_v.x < f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator==(float f) const
{
  return m_v.x == f;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator!=(float f) const
{
  return m_v.x != f;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>(float f) const
{
  return m_v.x > f;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>=(float f) const
{
  return m_v.x >= f;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<(float f) const
{
  return m_v.x < f;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<=(float f) const
{
  return m_v.x <= f;
}

template<ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetReciprocal() const
{
  return ezSimdFloat(1.0f / m_v.x);
}

template<ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetSqrt() const
{
  return ezSimdFloat(ezMath::Sqrt(m_v.x));
}

template<ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetInvSqrt() const
{
  return ezSimdFloat(1.0f / ezMath::Sqrt(m_v.x));
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::Max(const ezSimdFloat& f) const
{
  return m_v.CompMax(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::Min(const ezSimdFloat& f) const
{
  return m_v.CompMin(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::Abs() const
{
  return ezSimdFloat(ezMath::Abs(m_v.x));
}

