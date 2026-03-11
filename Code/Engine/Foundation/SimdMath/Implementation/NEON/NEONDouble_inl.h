#pragma once

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v.xy = vdupq_n_f64(ezMath::NaN<double>());
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(float f)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(static_cast<double>(f));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(double f)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(f);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(static_cast<double>(i));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezUInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(static_cast<double>(i));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezAngle a)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(a.GetRadian());
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadFloat v)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  // Convert first float to double and broadcast
  float x = vgetq_lane_f32(v, 0);
  m_v.xy = vdupq_n_f64(static_cast<double>(x));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadDouble v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdDouble::operator double() const
{
  return vgetq_lane_f64(m_v.xy, 0);
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeZero()
{
  ezSimdDouble result;
  result.m_v.xy = vdupq_n_f64(0.0);
  result.m_v.zw = result.m_v.xy;
  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeNaN()
{
  ezSimdDouble result;
  result.m_v.xy = vdupq_n_f64(ezMath::NaN<double>());
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator+(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = vaddq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator-(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = vsubq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator*(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = vmulq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator/(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = vdivq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator+=(const ezSimdDouble& f)
{
  m_v.xy = vaddq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator-=(const ezSimdDouble& f)
{
  m_v.xy = vsubq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator*=(const ezSimdDouble& f)
{
  m_v.xy = vmulq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator/=(const ezSimdDouble& f)
{
  m_v.xy = vdivq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::IsEqual(const ezSimdDouble& rhs, const ezSimdDouble& fEpsilon) const
{
  // Match SSE implementation: (this >= rhs - eps) && (this <= rhs + eps)
  ezSimdDouble minusEps = rhs - fEpsilon;
  ezSimdDouble plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(const ezSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) == vgetq_lane_f64(f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(const ezSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) != vgetq_lane_f64(f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(const ezSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) >= vgetq_lane_f64(f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(const ezSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) > vgetq_lane_f64(f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(const ezSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) <= vgetq_lane_f64(f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(const ezSimdDouble& f) const
{
  return vgetq_lane_f64(m_v.xy, 0) < vgetq_lane_f64(f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) == f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) != f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) > f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) >= f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) < f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(double f) const
{
  return vgetq_lane_f64(m_v.xy, 0) <= f;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) == static_cast<double>(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) != static_cast<double>(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) > static_cast<double>(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) >= static_cast<double>(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) < static_cast<double>(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(float f) const
{
  return vgetq_lane_f64(m_v.xy, 0) <= static_cast<double>(f);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal() const
{
  ezSimdDouble result;
  result.m_v.xy = vdivq_f64(vdupq_n_f64(1.0), m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt() const
{
  ezSimdDouble result;
  result.m_v.xy = vsqrtq_f64(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt() const
{
  ezSimdDouble result;
  result.m_v.xy = vdivq_f64(vdupq_n_f64(1.0), vsqrtq_f64(m_v.xy));
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Max(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = vmaxq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Min(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = vminq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Abs() const
{
  ezSimdDouble result;
  result.m_v.xy = vabsq_f64(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}
