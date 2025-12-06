#pragma once

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v.xy = _mm_set1_pd(ezMath::NaN<double>());
  m_v.zw = m_v.xy;
#endif


}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(float f)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(double(f));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(double d)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(d);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  __m128i packedInt32 = _mm_set1_epi32(i);
  m_v.xy = _mm_cvtepi32_pd(packedInt32);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezUInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(static_cast<double>(i));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezAngle a)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(a.GetRadian());
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadFloat v)
{
  m_v.xy = _mm_cvtps_pd(v);
  m_v.zw = _mm_cvtps_pd(_mm_movehl_ps(v, v));
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadDouble v)
{
  m_v = v;
}



// EZ_ALWAYS_INLINE ezSimdDouble::operator float() const
// {
//   double d;
//   _mm256_store_pd(&d, m_v);
//   return float(d);
// }

EZ_ALWAYS_INLINE ezSimdDouble::operator double() const
{
  return _mm_cvtsd_f64(m_v.xy);
}


// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeZero()
{
  ezSimdDouble result;
  result.m_v.xy = _mm_setzero_pd();
  result.m_v.zw = result.m_v.xy;
  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeNaN()
{
  ezSimdDouble result;
  result.m_v.xy = _mm_set1_pd(ezMath::NaN<double>());
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator+(const ezSimdDouble& d) const
{
  ezSimdDouble result;
  result.m_v.xy = _mm_add_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator-(const ezSimdDouble& d) const
{
  ezSimdDouble result;
  result.m_v.xy = _mm_sub_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator*(const ezSimdDouble& d) const
{
  ezSimdDouble result;
  result.m_v.xy = _mm_mul_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator/(const ezSimdDouble& d) const
{
  ezSimdDouble result;
  result.m_v.xy = _mm_div_pd(m_v.xy, d.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator+=(const ezSimdDouble& d)
{
  m_v.xy = _mm_add_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator-=(const ezSimdDouble& d)
{
  m_v.xy = _mm_sub_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator*=(const ezSimdDouble& d)
{
  m_v.xy = _mm_mul_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator/=(const ezSimdDouble& d)
{
  m_v.xy = _mm_div_pd(m_v.xy, d.m_v.xy);
  m_v.zw = m_v.xy;
  return *this;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::IsEqual(const ezSimdDouble& rhs, const ezSimdDouble& epsilon) const
{
  ezSimdDouble minusEps = rhs - epsilon;
  ezSimdDouble plusEps = rhs + epsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(const ezSimdDouble& d) const
{
  return _mm_comieq_sd(m_v.xy, d.m_v.xy) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(const ezSimdDouble& d) const
{
  return _mm_comineq_sd(m_v.xy, d.m_v.xy) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(const ezSimdDouble& d) const
{
  return _mm_comige_sd(m_v.xy, d.m_v.xy) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(const ezSimdDouble& d) const
{
  return _mm_comigt_sd(m_v.xy, d.m_v.xy) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(const ezSimdDouble& d) const
{
  return _mm_comile_sd(m_v.xy, d.m_v.xy) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(const ezSimdDouble& d) const
{
  return _mm_comilt_sd(m_v.xy, d.m_v.xy) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(double d) const
{
  return (*this) == ezSimdDouble(d);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(double d) const
{
  return (*this) != ezSimdDouble(d);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(double d) const
{
  return (*this) > ezSimdDouble(d);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(double d) const
{
  return (*this) >= ezSimdDouble(d);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(double d) const
{
  return (*this) < ezSimdDouble(d);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(double d) const
{
  return (*this) <= ezSimdDouble(d);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(float f) const
{
  return (*this) == ezSimdDouble(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(float f) const
{
  return (*this) != ezSimdDouble(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(float f) const
{
  return (*this) > ezSimdDouble(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(float f) const
{
  return (*this) >= ezSimdDouble(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(float f) const
{
  return (*this) < ezSimdDouble(f);
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(float f) const
{
  return (*this) <= ezSimdDouble(f);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal() const
{
  ezSimdDouble result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt() const
{
  ezSimdDouble result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, _mm_sqrt_pd(m_v.xy));
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt() const
{
  ezSimdDouble result;
  result.m_v.xy = _mm_sqrt_pd(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Max(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = _mm_max_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Min(const ezSimdDouble& f) const
{
  ezSimdDouble result;
  result.m_v.xy = _mm_min_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Abs() const
{
  ezSimdDouble result;
  __m128d sign_mask = _mm_set1_pd(-0.0);
  result.m_v.xy = _mm_andnot_pd(sign_mask, m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}
