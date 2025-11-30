#pragma once

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm_set1_ps(ezMath::NaN<float>());
#endif
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(float f)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(f);
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
  m_v = _mm_shuffle_ps(v, v, EZ_TO_SHUFFLE(ezSwizzle::XXXX));
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezUInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_PLATFORM_64BIT)
  __m128 v = _mm_cvtsi64_ss(_mm_setzero_ps(), i);
#else
  __m128 v = _mm_cvtsi32_ss(_mm_setzero_ps(), i);
#endif
  m_v = _mm_shuffle_ps(v, v, EZ_TO_SHUFFLE(ezSwizzle::XXXX));
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezAngle a)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_ps(a.GetRadian());
}

EZ_ALWAYS_INLINE ezSimdDouble::ezSimdDouble(ezInternal::QuadFloat v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdDouble::operator float() const
{
  float f;
  _mm_store_ss(&f, m_v);
  return f;
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeZero()
{
  return _mm_setzero_ps();
}

// static
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::MakeNaN()
{
  return _mm_set1_ps(ezMath::NaN<float>());
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator+(const ezSimdDouble& f) const
{
  return _mm_add_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator-(const ezSimdDouble& f) const
{
  return _mm_sub_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator*(const ezSimdDouble& f) const
{
  return _mm_mul_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::operator/(const ezSimdDouble& f) const
{
  return _mm_div_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator+=(const ezSimdDouble& f)
{
  m_v = _mm_add_ps(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator-=(const ezSimdDouble& f)
{
  m_v = _mm_sub_ps(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator*=(const ezSimdDouble& f)
{
  m_v = _mm_mul_ps(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdDouble& ezSimdDouble::operator/=(const ezSimdDouble& f)
{
  m_v = _mm_div_ps(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::IsEqual(const ezSimdDouble& rhs, const ezSimdDouble& fEpsilon) const
{
  ezSimdDouble minusEps = rhs - fEpsilon;
  ezSimdDouble plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator==(const ezSimdDouble& f) const
{
  return _mm_comieq_ss(m_v, f.m_v) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator!=(const ezSimdDouble& f) const
{
  return _mm_comineq_ss(m_v, f.m_v) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>=(const ezSimdDouble& f) const
{
  return _mm_comige_ss(m_v, f.m_v) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator>(const ezSimdDouble& f) const
{
  return _mm_comigt_ss(m_v, f.m_v) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<=(const ezSimdDouble& f) const
{
  return _mm_comile_ss(m_v, f.m_v) == 1;
}

EZ_ALWAYS_INLINE bool ezSimdDouble::operator<(const ezSimdDouble& f) const
{
  return _mm_comilt_ss(m_v, f.m_v) == 1;
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
  return _mm_div_ps(_mm_set1_ps(1.0f), m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal<ezMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rcp_ps(m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(m_v, x0)));

  return x1;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetReciprocal<ezMathAcc::BITS_12>() const
{
  return _mm_rcp_ps(m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt<ezMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), _mm_sqrt_ps(m_v));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt<ezMathAcc::BITS_23>() const
{
  const __m128 x0 = _mm_rsqrt_ps(m_v);

  // One iteration of Newton-Raphson
  return _mm_mul_ps(_mm_mul_ps(_mm_set1_ps(0.5f), x0), _mm_sub_ps(_mm_set1_ps(3.0f), _mm_mul_ps(_mm_mul_ps(m_v, x0), x0)));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetInvSqrt<ezMathAcc::BITS_12>() const
{
  return _mm_rsqrt_ps(m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::GetSqrt<ezMathAcc::FULL>() const
{
  return _mm_sqrt_ps(m_v);
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
  return _mm_max_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Min(const ezSimdDouble& f) const
{
  return _mm_min_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdDouble::Abs() const
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), m_v);
}
