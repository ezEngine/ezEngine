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

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt() const
{
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt() const
{
  return _mm256_sqrt_pd(m_v);
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
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0), m_v);
}
