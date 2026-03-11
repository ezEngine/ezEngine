#pragma once

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble() {}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(float f)
{
  m_v.Set((double)f);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(double f)
{
  m_v.Set(f);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInt32 i)
{
  m_v.Set((double)i);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezUInt32 i)
{
  m_v.Set((double)i);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezAngle a)
{
  m_v.Set(a.GetRadian());
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadFloat v)
{
  m_v.Set(v.x);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadDouble v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdDouble::operator double() const
{
  return m_v.x;
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeZero()
{
  return ezSimdDouble(0.0);
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeNaN()
{
  return ezSimdDouble(ezMath::NaN<double>());
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator+(const ezSimdDouble& f) const
{
  return m_v + f.m_v;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator-(const ezSimdDouble& f) const
{
  return m_v - f.m_v;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator*(const ezSimdDouble& f) const
{
  return m_v.CompMul(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator/(const ezSimdDouble& f) const
{
  return m_v.CompDiv(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator+=(const ezSimdDouble& f)
{
  m_v += f.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator-=(const ezSimdDouble& f)
{
  m_v -= f.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator*=(const ezSimdDouble& f)
{
  m_v = m_v.CompMul(f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator/=(const ezSimdDouble& f)
{
  m_v = m_v.CompDiv(f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::IsEqual(const ezSimdDouble& rhs, const ezSimdDouble& dEpsilon) const
{
  return m_v.IsEqual(rhs.m_v, dEpsilon);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(const ezSimdDouble& f) const
{
  return m_v.x == f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(const ezSimdDouble& f) const
{
  return m_v.x != f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(const ezSimdDouble& f) const
{
  return m_v.x >= f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(const ezSimdDouble& f) const
{
  return m_v.x > f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(const ezSimdDouble& f) const
{
  return m_v.x <= f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(const ezSimdDouble& f) const
{
  return m_v.x < f.m_v.x;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(double f) const
{
  return m_v.x == f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(double f) const
{
  return m_v.x != f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(double f) const
{
  return m_v.x > f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(double f) const
{
  return m_v.x >= f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(double f) const
{
  return m_v.x < f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(double f) const
{
  return m_v.x <= f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(float f) const
{
  return m_v.x == (double)f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(float f) const
{
  return m_v.x != (double)f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(float f) const
{
  return m_v.x > (double)f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(float f) const
{
  return m_v.x >= (double)f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(float f) const
{
  return m_v.x < (double)f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(float f) const
{
  return m_v.x <= (double)f;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal() const
{
  return ezSimdDouble(1.0 / m_v.x);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt() const
{
  return ezSimdDouble(ezMath::Sqrt(m_v.x));
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt() const
{
  return ezSimdDouble(1.0 / ezMath::Sqrt(m_v.x));
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Max(const ezSimdDouble& f) const
{
  return m_v.CompMax(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Min(const ezSimdDouble& f) const
{
  return m_v.CompMin(f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Abs() const
{
  return ezSimdDouble(ezMath::Abs(m_v.x));
}
