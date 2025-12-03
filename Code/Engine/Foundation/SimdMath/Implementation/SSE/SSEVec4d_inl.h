#pragma once

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_set1_pd(ezMath::NaN<double>());
#else
  m_v.xy = _mm_set1_pd(ezMath::NaN<double>());
  m_v.zw = m_v.xy;
#endif
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_set1_pd(static_cast<double>(fXyzw));
#else
  m_v.xy = _mm_set1_pd(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double dXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_set1_pd(dXyzw);
#else
  m_v.xy = _mm_set1_pd(dXyzw);
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(const ezSimdDouble& fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = fXyzw.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(ezInternal::QuadDouble v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float x, float y, float z, float w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_setr_pd(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w));
#else
  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double x, double y, double z, double w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_setr_pd(x, y, z, w);
#else
  m_v.xy = _mm_setr_pd(x, y);
  m_v.zw = _mm_setr_pd(z, w);
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float fXyzw)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_set1_pd(static_cast<double>(fXyzw));
#else
  m_v.xy = _mm_set1_pd(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double fXyzw)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_set1_pd(fXyzw);
#else
  m_v.xy = _mm_set1_pd(fXyzw);
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float x, float y, float z, float w)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_setr_pd(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w));
#else
  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double x, double y, double z, double w)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_setr_pd(x, y, z, w);
#else
  m_v.xy = _mm_setr_pd(x, y);
  m_v.zw = _mm_setr_pd(z, w);
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetX(const ezSimdDouble& f)
{
  m_v.xy = _mm_move_sd(m_v.xy, f.m_v.xy);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetY(const ezSimdDouble& f)
{
  m_v.xy = _mm_shuffle_pd(_mm_unpacklo_pd(m_v.xy, f.m_v.xy), m_v.xy, 0);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZ(const ezSimdDouble& f)
{
  m_v.zw = _mm_move_sd(m_v.zw, f.m_v.xy);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetW(const ezSimdDouble& f)
{
  m_v.zw = _mm_shuffle_pd(_mm_unpacklo_pd(m_v.zw, f.m_v.xy), m_v.zw, 0);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZero()
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_setzero_pd();
#else
  m_v.xy = _mm_setzero_pd();
  m_v.zw = m_v.xy;
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<1>(const float* pFloat)
{
  m_v.xy = _mm_cvtps_pd(_mm_load_ss(pFloat));
  m_v.zw = m_v.xy;
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<2>(const float* pFloat)
{
  __m128 temp = _mm_setr_ps(pFloat[0], pFloat[1], 0.0f, 0.0f);
  m_v.xy = _mm_cvtps_pd(temp);
  m_v.zw = m_v.xy;
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<3>(const float* pFloat)
{
  __m128 temp = _mm_set_ps(0.0f, pFloat[2], pFloat[1], pFloat[0]);
  m_v.xy = _mm_cvtps_pd(temp);
  m_v.zw = _mm_cvtps_pd(_mm_movehl_ps(temp, temp));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<4>(const float* pFloat)
{
  __m128 temp = _mm_loadu_ps(pFloat);
  m_v.xy = _mm_cvtps_pd(temp);
  m_v.zw = _mm_cvtps_pd(_mm_movehl_ps(temp, temp));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<1>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_broadcast_sd(pDouble);
#else
  m_v.xy = _mm_load_sd(pDouble);
  m_v.zw = m_v.xy;
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<2>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_broadcast_sd(pDouble);
#else
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = m_v.xy;
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<3>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_setr_pd(pDouble[0], pDouble[1], pDouble[2], 0.0);
#else
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = _mm_load_sd(pDouble + 2);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<4>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  m_v = _mm256_loadu_pd(pDouble);
#else
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = _mm_loadu_pd(pDouble + 2);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<1>(float* pFloat) const
{
  __m128 temp = _mm_cvtpd_ps(m_v.xy);
  _mm_store_ss(pFloat, temp);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<2>(float* pFloat) const
{
  __m128 temp = _mm_cvtpd_ps(m_v.xy);
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(temp));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<3>(float* pFloat) const
{
  __m128 temp_xy = _mm_cvtpd_ps(m_v.xy);
  __m128 temp_zw = _mm_cvtpd_ps(m_v.zw);
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(temp_xy));
  _mm_store_ss(pFloat + 2, temp_zw);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<4>(float* pFloat) const
{
  __m128 temp_xy = _mm_cvtpd_ps(m_v.xy);
  __m128 temp_zw = _mm_cvtpd_ps(m_v.zw);
  __m128 result = _mm_movelh_ps(temp_xy, temp_zw);
  _mm_storeu_ps(pFloat, result);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<1>(double* pDouble) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  _mm_store_sd(pDouble, _mm256_castpd256_pd128(m_v));
#else
  _mm_store_sd(pDouble, m_v.xy);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<2>(double* pDouble) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  _mm_storeu_pd(pDouble, _mm256_castpd256_pd128(m_v));
#else
  _mm_storeu_pd(pDouble, m_v.xy);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<3>(double* pDouble) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  _mm_storeu_pd(pDouble, _mm256_castpd256_pd128(m_v));
  _mm_store_sd(pDouble + 2, _mm256_extractf128_pd(m_v, 1));
#else
  _mm_storeu_pd(pDouble, m_v.xy);
  _mm_store_sd(pDouble + 2, m_v.zw);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<4>(double* pDouble) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  _mm256_storeu_pd(pDouble, m_v);
#else
  _mm_storeu_pd(pDouble, m_v.xy);
  _mm_storeu_pd(pDouble + 2, m_v.zw);
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal<ezMathAcc::BITS_12>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  // Convert double to float, get fast reciprocal, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatRcp = _mm_rcp_ps(floatVal);
  return _mm256_cvtps_pd(floatRcp);
#else
  // Convert double to float, get fast reciprocal, convert back to double
  __m128 floatVal = _mm_cvtpd_ps(m_v.xy);
  __m128 floatRcp = _mm_rcp_ps(floatVal);
  
  ezSimdVec4d result;
  result.m_v.xy = _mm_cvtps_pd(floatRcp);
  result.m_v.zw = result.m_v.xy;
  return result;

  //TODO check if precision adequate.
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal<ezMathAcc::BITS_23>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  // Convert double to float, get reciprocal approximation, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatRcp = _mm_rcp_ps(floatVal);
  __m256d x0 = _mm256_cvtps_pd(floatRcp);

  // One iteration of Newton-Raphson: x1 = x0 * (2 - m_v * x0)
  __m256d two = _mm256_set1_pd(2.0);
  return _mm256_mul_pd(x0, _mm256_sub_pd(two, _mm256_mul_pd(m_v, x0)));
#else
  // Convert double to float, get reciprocal approximation, convert back to double
  __m128 floatVal = _mm_cvtpd_ps(m_v.xy);
  __m128 floatRcp = _mm_rcp_ps(floatVal);
  
  ezSimdVec4d result;
  result.m_v.xy = _mm_cvtps_pd(floatRcp);

  // One iteration of Newton-Raphson: x1 = x0 * (2 - m_v * x0)
  __m128d two = _mm_set1_pd(2.0);
  result.m_v.xy = _mm_mul_pd(result.m_v.xy, _mm_sub_pd(two, _mm_mul_pd(m_v.xy, result.m_v.xy)));
  result.m_v.zw = result.m_v.xy;

  return result;
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal<ezMathAcc::FULL>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
#else
  ezSimdVec4d result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetSqrt<ezMathAcc::BITS_12>() const
{
  return (*this) * GetInvSqrt<ezMathAcc::BITS_12>();
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetSqrt<ezMathAcc::BITS_23>() const
{
  return (*this) * GetInvSqrt<ezMathAcc::BITS_23>();
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetSqrt<ezMathAcc::FULL>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_sqrt_pd(m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_sqrt_pd(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt<ezMathAcc::FULL>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
#else
  ezSimdVec4d result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, _mm_sqrt_pd(m_v.xy));
  result.m_v.zw = result.m_v.xy;
  return result;
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt<ezMathAcc::BITS_23>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  // Convert double to float, get inverse sqrt approximation, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatInvSqrt = _mm_rsqrt_ps(floatVal);
  __m256d x0 = _mm256_cvtps_pd(floatInvSqrt);

  // One iteration of Newton-Raphson: x1 = 0.5 * x0 * (3 - m_v * x0 * x0)
  __m256d half = _mm256_set1_pd(0.5);
  __m256d three = _mm256_set1_pd(3.0);
  __m256d x0_squared = _mm256_mul_pd(x0, x0);
  __m256d three_minus = _mm256_sub_pd(three, _mm256_mul_pd(m_v, x0_squared));
  return _mm256_mul_pd(_mm256_mul_pd(half, x0), three_minus);
#else
  // Convert double to float, get inverse sqrt approximation, convert back to double
  __m128 floatVal = _mm_cvtpd_ps(m_v.xy);
  __m128 floatInvSqrt = _mm_rsqrt_ps(floatVal);
  
  ezSimdVec4d result;
  result.m_v.xy = _mm_cvtps_pd(floatInvSqrt);

  // One iteration of Newton-Raphson: x1 = 0.5 * x0 * (3 - m_v * x0 * x0)
  __m128d half = _mm_set1_pd(0.5);
  __m128d three = _mm_set1_pd(3.0);
  
  __m128d x0_squared = _mm_mul_pd(result.m_v.xy, result.m_v.xy);
  __m128d three_minus = _mm_sub_pd(three, _mm_mul_pd(m_v.xy, x0_squared));
  result.m_v.xy = _mm_mul_pd(_mm_mul_pd(half, result.m_v.xy), three_minus);
  result.m_v.zw = result.m_v.xy;
  
  return result;
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt<ezMathAcc::BITS_12>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  // Convert double to float, get fast inverse sqrt approximation, convert back to double
  __m128 floatVal = _mm256_cvtpd_ps(m_v);
  __m128 floatInvSqrt = _mm_rsqrt_ps(floatVal);
  return _mm256_cvtps_pd(floatInvSqrt);
#else
  // Convert double to float, get fast inverse sqrt approximation, convert back to double
  __m128 floatVal = _mm_cvtpd_ps(m_v.xy);
  __m128 floatInvSqrt = _mm_rsqrt_ps(floatVal);
  
  ezSimdVec4d result;
  result.m_v.xy = _mm_cvtps_pd(floatInvSqrt);
  result.m_v.zw = result.m_v.xy;
  return result;
#endif
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetComponent() const
{
  if constexpr (N == 0)
  {
    ezSimdDouble result;
    result.m_v.xy = m_v.xy;
    result.m_v.zw = result.m_v.xy;
    return result;
  }
  else if constexpr (N == 1)
  {
    ezSimdDouble result;
    result.m_v.xy = _mm_shuffle_pd(m_v.xy, m_v.xy, _MM_SHUFFLE2(1, 1));
    result.m_v.zw = result.m_v.xy;
    return result;
  }
  else if constexpr (N == 2)
  {
    ezSimdDouble result;
    result.m_v.xy = m_v.zw;
    result.m_v.zw = result.m_v.xy;
    return result;
  }
  else // N == 3
  {
    ezSimdDouble result;
    result.m_v.xy = _mm_shuffle_pd(m_v.zw, m_v.zw, _MM_SHUFFLE2(1, 1));
    result.m_v.zw = result.m_v.xy;
    return result;
  }
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::x() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::y() const
{
  return GetComponent<1>();
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::z() const
{
  return GetComponent<2>();
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::w() const
{
  return GetComponent<3>();
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator-() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_sub_pd(_mm256_setzero_pd(), m_v);
#else
  ezSimdVec4d result;
  __m128d zero = _mm_setzero_pd();
  result.m_v.xy = _mm_sub_pd(zero, m_v.xy);
  result.m_v.zw = _mm_sub_pd(zero, m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator+(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_add_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_add_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_add_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator-(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_sub_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_sub_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_sub_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator*(const ezSimdDouble& f) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_mul_pd(m_v, f.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_mul_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = _mm_mul_pd(m_v.zw, f.m_v.xy);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator/(const ezSimdDouble& f) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_div_pd(m_v, f.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_div_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = _mm_div_pd(m_v.zw, f.m_v.xy);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMul(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_mul_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_mul_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_mul_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompDiv<ezMathAcc::FULL>(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_div_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_div_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_div_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompDiv<ezMathAcc::BITS_23>(const ezSimdVec4d& v) const
{
  return CompMul(v.GetReciprocal<ezMathAcc::BITS_23>());
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompDiv<ezMathAcc::BITS_12>(const ezSimdVec4d& v) const
{
  return CompMul(v.GetReciprocal<ezMathAcc::BITS_12>());
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMin(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_min_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_min_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_min_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMax(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_max_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_max_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_max_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Abs() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0), m_v);
#else
  ezSimdVec4d result;
  __m128d signMask = _mm_set1_pd(-0.0);
  result.m_v.xy = _mm_andnot_pd(signMask, m_v.xy);
  result.m_v.zw = _mm_andnot_pd(signMask, m_v.zw);
  return result;
#endif
}