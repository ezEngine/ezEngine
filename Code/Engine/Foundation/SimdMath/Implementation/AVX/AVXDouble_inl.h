#pragma once

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm256_set1_pd(ezMath::NaN<double>());
#endif


}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(float f)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(double(f));
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(double d)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(d);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  __m128i packedInt32 = _mm_set1_epi32(i);
  m_v = _mm256_cvtepi32_pd(packedInt32);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezUInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(static_cast<double>(i));
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezAngle a)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm256_set1_pd(a.GetRadian());
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadFloat v)
{
  m_v = _mm256_cvtps_pd(v);
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
  return _mm_cvtsd_f64(_mm256_castpd256_pd128(m_v));
}


// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeZero()
{
  return _mm256_setzero_pd();
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeNaN()
{
  return _mm256_set1_pd(ezMath::NaN<double>());
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator+(const ezSimdDouble& d) const
{
  return _mm256_add_pd(m_v, d.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator-(const ezSimdDouble& d) const
{
  return _mm256_sub_pd(m_v, d.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator*(const ezSimdDouble& d) const
{
  return _mm256_mul_pd(m_v, d.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator/(const ezSimdDouble& d) const
{
  return _mm256_div_pd(m_v, d.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator+=(const ezSimdDouble& d)
{
  m_v = _mm256_add_pd(m_v, d.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator-=(const ezSimdDouble& d)
{
  m_v = _mm256_sub_pd(m_v, d.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator*=(const ezSimdDouble& d)
{
  m_v = _mm256_mul_pd(m_v, d.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator/=(const ezSimdDouble& d)
{
  m_v = _mm256_div_pd(m_v, d.m_v);
  return *this;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::IsEqual(const ezSimdDouble& rhs, const ezSimdDouble& fEpsilon) const
{
  ezSimdDouble minusEps = rhs - fEpsilon;
  ezSimdDouble plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(const ezSimdDouble& d) const
{
  return _mm_comieq_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(const ezSimdDouble& d) const
{
  return _mm_comineq_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(const ezSimdDouble& d) const
{
  return _mm_comige_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(const ezSimdDouble& d) const
{
  return _mm_comigt_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(const ezSimdDouble& d) const
{
  return _mm_comile_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(const ezSimdDouble& d) const
{
  return _mm_comilt_sd(_mm256_castpd256_pd128(m_v), _mm256_castpd256_pd128(d.m_v)) == 1;
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

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal<ezMathAcc::FULL>() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal<ezMathAcc::BITS_23>() const
{
  // Convert double to float, get reciprocal approximation, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatRcp = _mm_rcp_ps(floatVal);
  __m256d x0 = _mm256_cvtps_pd(floatRcp);

  // One iteration of Newton-Raphson: x1 = x0 * (2 - m_v * x0)
  __m256d x1 = _mm256_mul_pd(x0, _mm256_sub_pd(_mm256_set1_pd(2.0), _mm256_mul_pd(m_v, x0)));

  return x1;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal<ezMathAcc::BITS_12>() const
{
  // Convert double to float, get fast reciprocal approximation, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatRcp = _mm_rcp_ps(floatVal);
  return _mm256_cvtps_pd(floatRcp);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt<ezMathAcc::FULL>() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt<ezMathAcc::BITS_23>() const
{
  // Convert double to float, get inverse sqrt approximation, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatInvSqrt = _mm_rsqrt_ps(floatVal);
  __m256d x0 = _mm256_cvtps_pd(floatInvSqrt);

  // One iteration of Newton-Raphson: x1 = 0.5 * x0 * (3 - m_v * x0 * x0)
  __m256d x0_squared = _mm256_mul_pd(x0, x0);
  __m256d three_minus = _mm256_sub_pd(_mm256_set1_pd(3.0), _mm256_mul_pd(m_v, x0_squared));
  return _mm256_mul_pd(_mm256_mul_pd(_mm256_set1_pd(0.5), x0), three_minus);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt<ezMathAcc::BITS_12>() const
{
  // Convert double to float, get fast inverse sqrt approximation, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatInvSqrt = _mm_rsqrt_ps(floatVal);
  return _mm256_cvtps_pd(floatInvSqrt);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt<ezMathAcc::FULL>() const
{
  return _mm256_sqrt_pd(m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt<ezMathAcc::BITS_23>() const
{
  return (*this) * GetInvSqrt<ezMathAcc::BITS_23>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt<ezMathAcc::BITS_12>() const
{
  return (*this) * GetInvSqrt<ezMathAcc::BITS_12>();
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Max(const ezSimdDouble& f) const
{
  return _mm256_max_pd(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Min(const ezSimdDouble& f) const
{
  return _mm256_min_pd(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Abs() const
{
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0d), m_v);
}
