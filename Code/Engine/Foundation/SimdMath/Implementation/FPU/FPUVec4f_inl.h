#pragma once

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f() {}

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

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetX(const ezSimdFloat& f)
{
  m_v.x = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetY(const ezSimdFloat& f)
{
  m_v.y = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetZ(const ezSimdFloat& f)
{
  m_v.z = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetW(const ezSimdFloat& f)
{
  m_v.w = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetZero()
{
  m_v.SetZero();
}

template <int N>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load(const float* pFloats)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pFloats[i];
  }
}

template <int N>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store(float* pFloats) const
{
  for (int i = 0; i < N; ++i)
  {
    pFloats[i] = (&m_v.x)[i];
  }
}

template <ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetReciprocal() const
{
  return ezVec4(1.0f).CompDiv(m_v);
}

template <ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetSqrt() const
{
  ezSimdVec4f result;
  result.m_v.x = ezMath::Sqrt(m_v.x);
  result.m_v.y = ezMath::Sqrt(m_v.y);
  result.m_v.z = ezMath::Sqrt(m_v.z);
  result.m_v.w = ezMath::Sqrt(m_v.w);

  return result;
}

template <int N, ezMathAcc::Enum acc>
void ezSimdVec4f::NormalizeIfNotZero(const ezSimdFloat& fEpsilon)
{
  ezSimdFloat sqLength = GetLengthSquared<N>();
  m_v *= sqLength.GetInvSqrt<acc>();
  m_v = sqLength > fEpsilon.m_v ? m_v : ezVec4::ZeroVector();
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsZero() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i] != 0.0f)
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsZero(const ezSimdFloat& fEpsilon) const
{
  for (int i = 0; i < N; ++i)
  {
    if (!ezMath::IsZero((&m_v.x)[i], (float)fEpsilon))
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsNaN() const
{
  for (int i = 0; i < N; ++i)
  {
    if (ezMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsValid() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!ezMath::IsFinite((&m_v.x)[i]))
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetComponent() const
{
  return (&m_v.x)[N];
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

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Get() const
{
  ezSimdVec4f result;

  const float* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetCombined(const ezSimdVec4f& other) const
{
  ezSimdVec4f result;

  const float* v = &m_v.x;
  const float* o = &other.m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = o[(s & 0x0030) >> 4];
  result.m_v.w = o[(s & 0x0003)];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator-() const
{
  return -m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator+(const ezSimdVec4f& v) const
{
  return m_v + v.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator-(const ezSimdVec4f& v) const
{
  return m_v - v.m_v;
}


EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator*(const ezSimdFloat& f) const
{
  return m_v * f.m_v.x;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator/(const ezSimdFloat& f) const
{
  return m_v / f.m_v.x;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMul(const ezSimdVec4f& v) const
{
  return m_v.CompMul(v.m_v);
}

template <ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompDiv(const ezSimdVec4f& v) const
{
  return m_v.CompDiv(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMin(const ezSimdVec4f& v) const
{
  return m_v.CompMin(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMax(const ezSimdVec4f& v) const
{
  return m_v.CompMax(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Abs() const
{
  return m_v.Abs();
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Floor() const
{
  ezSimdVec4f result;
  result.m_v.x = ezMath::Floor(m_v.x);
  result.m_v.y = ezMath::Floor(m_v.y);
  result.m_v.z = ezMath::Floor(m_v.z);
  result.m_v.w = ezMath::Floor(m_v.w);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Ceil() const
{
  ezSimdVec4f result;
  result.m_v.x = ezMath::Ceil(m_v.x);
  result.m_v.y = ezMath::Ceil(m_v.y);
  result.m_v.z = ezMath::Ceil(m_v.z);
  result.m_v.w = ezMath::Ceil(m_v.w);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::FlipSign(const ezSimdVec4b& cmp) const
{
  ezSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? -m_v.x : m_v.x;
  result.m_v.y = cmp.m_v.y ? -m_v.y : m_v.y;
  result.m_v.z = cmp.m_v.z ? -m_v.z : m_v.z;
  result.m_v.w = cmp.m_v.w ? -m_v.w : m_v.w;

  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Select(const ezSimdVec4f& ifFalse, const ezSimdVec4f& ifTrue, const ezSimdVec4b& cmp)
{
  ezSimdVec4f result;
  result.m_v.x = cmp.m_v.x ? ifTrue.m_v.x : ifFalse.m_v.x;
  result.m_v.y = cmp.m_v.y ? ifTrue.m_v.y : ifFalse.m_v.y;
  result.m_v.z = cmp.m_v.z ? ifTrue.m_v.z : ifFalse.m_v.z;
  result.m_v.w = cmp.m_v.w ? ifTrue.m_v.w : ifFalse.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator+=(const ezSimdVec4f& v)
{
  m_v += v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator-=(const ezSimdVec4f& v)
{
  m_v -= v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator*=(const ezSimdFloat& f)
{
  m_v *= f.m_v.x;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator/=(const ezSimdFloat& f)
{
  m_v /= f.m_v.x;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator==(const ezSimdVec4f& v) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x == v.m_v.x;
  result.m_v.y = m_v.y == v.m_v.y;
  result.m_v.z = m_v.z == v.m_v.z;
  result.m_v.w = m_v.w == v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator!=(const ezSimdVec4f& v) const
{
  return !(*this == v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator<=(const ezSimdVec4f& v) const
{
  return !(*this > v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator<(const ezSimdVec4f& v) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x < v.m_v.x;
  result.m_v.y = m_v.y < v.m_v.y;
  result.m_v.z = m_v.z < v.m_v.z;
  result.m_v.w = m_v.w < v.m_v.w;

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator>=(const ezSimdVec4f& v) const
{
  return !(*this < v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator>(const ezSimdVec4f& v) const
{
  ezSimdVec4b result;
  result.m_v.x = m_v.x > v.m_v.x;
  result.m_v.y = m_v.y > v.m_v.y;
  result.m_v.z = m_v.z > v.m_v.z;
  result.m_v.w = m_v.w > v.m_v.w;

  return result;
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<2>() const
{
  return m_v.x + m_v.y;
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<3>() const
{
  return (float)HorizontalSum<2>() + m_v.z;
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<4>() const
{
  return (float)HorizontalSum<3>() + m_v.w;
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<2>() const
{
  return ezMath::Min(m_v.x, m_v.y);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<3>() const
{
  return ezMath::Min((float)HorizontalMin<2>(), m_v.z);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<4>() const
{
  return ezMath::Min((float)HorizontalMin<3>(), m_v.w);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<2>() const
{
  return ezMath::Max(m_v.x, m_v.y);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<3>() const
{
  return ezMath::Max((float)HorizontalMax<2>(), m_v.z);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<4>() const
{
  return ezMath::Max((float)HorizontalMax<3>(), m_v.w);
}

template <int N>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot(const ezSimdVec4f& v) const
{
  float result = 0.0f;

  for (int i = 0; i < N; ++i)
  {
    result += (&m_v.x)[i] * (&v.m_v.x)[i];
  }

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Cross(const ezSimdVec4f& v) const
{
  return m_v.GetAsVec3().Cross(v.m_v.GetAsVec3()).GetAsVec4(0.0f);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetOrthogonalVector() const
{
  if (ezMath::Abs(m_v.y) < 0.99f)
  {
    return ezVec4(-m_v.z, 0.0f, m_v.x, 0.0f);
  }
  else
  {
    return ezVec4(0.0f, m_v.z, -m_v.y, 0.0f);
  }
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::ZeroVector()
{
  return ezVec4::ZeroVector();
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulAdd(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
  return a.CompMul(b) + c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulAdd(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c)
{
  return a * b + c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulSub(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
  return a.CompMul(b) - c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulSub(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c)
{
  return a * b - c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CopySign(const ezSimdVec4f& magnitude, const ezSimdVec4f& sign)
{
  ezSimdVec4f result;
  result.m_v.x = sign.m_v.x < 0.0f ? -magnitude.m_v.x : magnitude.m_v.x;
  result.m_v.y = sign.m_v.y < 0.0f ? -magnitude.m_v.y : magnitude.m_v.y;
  result.m_v.z = sign.m_v.z < 0.0f ? -magnitude.m_v.z : magnitude.m_v.z;
  result.m_v.w = sign.m_v.w < 0.0f ? -magnitude.m_v.w : magnitude.m_v.w;

  return result;
}
