#pragma once

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d() = default;

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float xyzw)
{
  m_v.Set(double(xyzw));
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(const ezSimdDouble& xyzw)
{
  m_v = xyzw.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double x, double y, double z, double w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(int x, int y, int z, int w)
{
  m_v.Set(double(x), double(y), double(z), double(w));
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double xyzw)
{
  m_v.Set(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float xyzw)
{
  m_v.Set(double(xyzw));
}


EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float x, float y, float z, float w)
{
  m_v.Set(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double x, double y, double z, double w)
{
  m_v.Set(float(x), float(y), float(z), float(w));
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(int x, int y, int z, int w)
{
  m_v.Set(double(x), double(y), double(z), double(w));
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetX(const ezSimdDouble& f)
{
  m_v.x = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetY(const ezSimdDouble& f)
{
  m_v.y = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZ(const ezSimdDouble& f)
{
  m_v.z = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetW(const ezSimdDouble& f)
{
  m_v.w = f.m_v.x;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZero()
{
  m_v.SetZero();
}

template <int N>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load(const double* pDoubles)
{
  m_v.SetZero();
  for (int i = 0; i < N; ++i)
  {
    (&m_v.x)[i] = pDoubles[i];
  }
}

template <int N>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store(double* pDoubles) const
{
  for (int i = 0; i < N; ++i)
  {
    pDoubles[i] = (&m_v.x)[i];
  }
}


EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal() const
{
  return ezVec4d(1.0).CompDiv(m_v);
}


EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetSqrt() const
{
  ezSimdVec4d result;
  result.m_v.x = ezMath::Sqrt(m_v.x);
  result.m_v.y = ezMath::Sqrt(m_v.y);
  result.m_v.z = ezMath::Sqrt(m_v.z);
  result.m_v.w = ezMath::Sqrt(m_v.w);

  return result;
}


EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt() const
{
  ezSimdVec4d result;
  result.m_v.x = 1.0 / ezMath::Sqrt(m_v.x);
  result.m_v.y = 1.0 / ezMath::Sqrt(m_v.y);
  result.m_v.z = 1.0 / ezMath::Sqrt(m_v.z);
  result.m_v.w = 1.0 / ezMath::Sqrt(m_v.w);

  return result;
}

template <int N>
void ezSimdVec4d::NormalizeIfNotZero(const ezSimdDouble& fEpsilon)
{
  ezSimdDouble sqLength = GetLengthSquared<N>();
  m_v *= sqLength.GetInvSqrt();
  m_v = sqLength > fEpsilon.m_v ? m_v : ezVec4d::MakeZero();
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsZero() const
{
  for (int i = 0; i < N; ++i)
  {
    if ((&m_v.x)[i] != 0.0)
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsZero(const ezSimdDouble& fEpsilon) const
{
  for (int i = 0; i < N; ++i)
  {
    if (!ezMath::IsZero((&m_v.x)[i], (double)fEpsilon))
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsNaN() const
{
  for (int i = 0; i < N; ++i)
  {
    if (ezMath::IsNaN((&m_v.x)[i]))
      return true;
  }

  return false;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsValid() const
{
  for (int i = 0; i < N; ++i)
  {
    if (!ezMath::IsFinite((&m_v.x)[i]))
      return false;
  }

  return true;
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetComponent() const
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

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::x() const
{
  return m_v.x;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::y() const
{
  return m_v.y;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::z() const
{
  return m_v.z;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::w() const
{
  return m_v.w;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Get() const
{
  ezSimdVec4d result;

  const double* v = &m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = v[(s & 0x0030) >> 4];
  result.m_v.w = v[(s & 0x0003)];

  return result;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetCombined(const ezSimdVec4d& other) const
{
  ezSimdVec4d result;

  const double* v = &m_v.x;
  const double* o = &other.m_v.x;
  result.m_v.x = v[(s & 0x3000) >> 12];
  result.m_v.y = v[(s & 0x0300) >> 8];
  result.m_v.z = o[(s & 0x0030) >> 4];
  result.m_v.w = o[(s & 0x0003)];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator-() const
{
  return -m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator+(const ezSimdVec4d& v) const
{
  return m_v + v.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator-(const ezSimdVec4d& v) const
{
  return m_v - v.m_v;
}


EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator*(const ezSimdDouble& f) const
{
  return m_v * f.m_v.x;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator/(const ezSimdDouble& f) const
{
  return m_v / f.m_v.x;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMul(const ezSimdVec4d& v) const
{
  return m_v.CompMul(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompDiv(const ezSimdVec4d& v) const
{
  return m_v.CompDiv(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMin(const ezSimdVec4d& v) const
{
  return m_v.CompMin(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMax(const ezSimdVec4d& v) const
{
  return m_v.CompMax(v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Abs() const
{
  return m_v.Abs();
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Round() const
{
  ezSimdVec4d result;
  result.m_v.x = ezMath::Round(m_v.x);
  result.m_v.y = ezMath::Round(m_v.y);
  result.m_v.z = ezMath::Round(m_v.z);
  result.m_v.w = ezMath::Round(m_v.w);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Floor() const
{
  ezSimdVec4d result;
  result.m_v.x = ezMath::Floor(m_v.x);
  result.m_v.y = ezMath::Floor(m_v.y);
  result.m_v.z = ezMath::Floor(m_v.z);
  result.m_v.w = ezMath::Floor(m_v.w);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Ceil() const
{
  ezSimdVec4d result;
  result.m_v.x = ezMath::Ceil(m_v.x);
  result.m_v.y = ezMath::Ceil(m_v.y);
  result.m_v.z = ezMath::Ceil(m_v.z);
  result.m_v.w = ezMath::Ceil(m_v.w);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Trunc() const
{
  ezSimdVec4d result;
  result.m_v.x = ezMath::Trunc(m_v.x);
  result.m_v.y = ezMath::Trunc(m_v.y);
  result.m_v.z = ezMath::Trunc(m_v.z);
  result.m_v.w = ezMath::Trunc(m_v.w);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::FlipSign(const ezSimdVec4bWide& cmp) const
{
  ezSimdVec4d r;
  r.m_v.x = vgetq_lane_u64(cmp.m_v.xy, 0) ? -m_v.x : m_v.x;
  r.m_v.y = vgetq_lane_u64(cmp.m_v.xy, 1) ? -m_v.y : m_v.y;
  r.m_v.z = vgetq_lane_u64(cmp.m_v.zw, 0) ? -m_v.z : m_v.z;
  r.m_v.w = vgetq_lane_u64(cmp.m_v.zw, 1) ? -m_v.w : m_v.w;
  return r;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Select(const ezSimdVec4bWide& cmp, const ezSimdVec4d& ifTrue, const ezSimdVec4d& ifFalse)
{
  ezSimdVec4d r;
  r.m_v.x = vgetq_lane_u64(cmp.m_v.xy, 0) ? ifTrue.m_v.x : ifFalse.m_v.x;
  r.m_v.y = vgetq_lane_u64(cmp.m_v.xy, 1) ? ifTrue.m_v.y : ifFalse.m_v.y;
  r.m_v.z = vgetq_lane_u64(cmp.m_v.zw, 0) ? ifTrue.m_v.z : ifFalse.m_v.z;
  r.m_v.w = vgetq_lane_u64(cmp.m_v.zw, 1) ? ifTrue.m_v.w : ifFalse.m_v.w;
  return r;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator+=(const ezSimdVec4d& v)
{
  m_v += v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator-=(const ezSimdVec4d& v)
{
  m_v -= v.m_v;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator*=(const ezSimdDouble& f)
{
  m_v *= f.m_v.x;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator/=(const ezSimdDouble& f)
{
  m_v /= f.m_v.x;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator==(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide r;
  r.m_v.xy = vceqq_f64(vld1q_f64(&m_v.x), vld1q_f64(&v.m_v.x));
  r.m_v.zw = vceqq_f64(vld1q_f64(&m_v.z), vld1q_f64(&v.m_v.z));
  return r;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator!=(const ezSimdVec4d& v) const
{
  return !(*this == v);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator<=(const ezSimdVec4d& v) const
{
  return !(*this > v);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator<(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide r;
  r.m_v.xy = vcltq_f64(vld1q_f64(&m_v.x), vld1q_f64(&v.m_v.x));
  r.m_v.zw = vcltq_f64(vld1q_f64(&m_v.z), vld1q_f64(&v.m_v.z));
  return r;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator>=(const ezSimdVec4d& v) const
{
  return !(*this < v);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator>(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide r;
  r.m_v.xy = vcgtq_f64(vld1q_f64(&m_v.x), vld1q_f64(&v.m_v.x));
  r.m_v.zw = vcgtq_f64(vld1q_f64(&m_v.z), vld1q_f64(&v.m_v.z));
  return r;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<2>() const
{
  return m_v.x + m_v.y;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<3>() const
{
  return (double)HorizontalSum<2>() + m_v.z;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<4>() const
{
  return (double)HorizontalSum<3>() + m_v.w;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<2>() const
{
  return ezMath::Min(m_v.x, m_v.y);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<3>() const
{
  return ezMath::Min((double)HorizontalMin<2>(), m_v.z);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<4>() const
{
  return ezMath::Min((double)HorizontalMin<3>(), m_v.w);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<2>() const
{
  return ezMath::Max(m_v.x, m_v.y);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<3>() const
{
  return ezMath::Max((double)HorizontalMax<2>(), m_v.z);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<4>() const
{
  return ezMath::Max((double)HorizontalMax<3>(), m_v.w);
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot(const ezSimdVec4d& v) const
{
  double result = 0.0;

  for (int i = 0; i < N; ++i)
  {
    result += (&m_v.x)[i] * (&v.m_v.x)[i];
  }

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CrossRH(const ezSimdVec4d& v) const
{
  return m_v.GetAsVec3().CrossRH(v.m_v.GetAsVec3()).GetAsVec4(0.0);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulAdd(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c)
{
  return a.CompMul(b) + c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulAdd(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c)
{
  return a * b + c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulSub(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c)
{
  return a.CompMul(b) - c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulSub(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c)
{
  return a * b - c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CopySign(const ezSimdVec4d& magnitude, const ezSimdVec4d& sign)
{
  ezSimdVec4d result;
  result.m_v.x = sign.m_v.x < 0.0 ? -magnitude.m_v.x : magnitude.m_v.x;
  result.m_v.y = sign.m_v.y < 0.0 ? -magnitude.m_v.y : magnitude.m_v.y;
  result.m_v.z = sign.m_v.z < 0.0 ? -magnitude.m_v.z : magnitude.m_v.z;
  result.m_v.w = sign.m_v.w < 0.0 ? -magnitude.m_v.w : magnitude.m_v.w;

  return result;
}