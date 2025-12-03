#pragma once

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v.xy = _mm_set1_pd(ezMath::NaN<double>());
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_set1_pd(fXyzw);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(const ezSimdDouble& fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = fXyzw.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float x, float y, float z, float w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double x, double y, double z, double w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = _mm_setr_pd(x, y);
  m_v.zw = _mm_setr_pd(z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float fXyzw)
{
  m_v.xy = _mm_set1_pd(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double fXyzw)
{
  m_v.xy = _mm_set1_pd(fXyzw);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float x, float y, float z, float w)
{
  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double x, double y, double z, double w)
{
  m_v.xy = _mm_setr_pd(x, y);
  m_v.zw = _mm_setr_pd(z, w);
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
  m_v.xy = _mm_setzero_pd();
  m_v.zw = m_v.xy;
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
  m_v.xy = _mm_load_sd(pDouble);
  m_v.zw = m_v.xy;
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<2>(const double* pDouble)
{
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = m_v.xy;
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<3>(const double* pDouble)
{
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = _mm_load_sd(pDouble + 2);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<4>(const double* pDouble)
{
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = _mm_loadu_pd(pDouble + 2);
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
  _mm_store_sd(pDouble, m_v.xy);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<2>(double* pDouble) const
{
  _mm_storeu_pd(pDouble, m_v.xy);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<3>(double* pDouble) const
{
  _mm_storeu_pd(pDouble, m_v.xy);
  _mm_store_sd(pDouble + 2, m_v.zw);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<4>(double* pDouble) const
{
  _mm_storeu_pd(pDouble, m_v.xy);
  _mm_storeu_pd(pDouble + 2, m_v.zw);
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal<ezMathAcc::BITS_12>() const
{

  // Convert double to float, get fast reciprocal, convert back to double
  __m128 floatVal = _mm_cvtpd_ps(m_v.xy);
  __m128 floatRcp = _mm_rcp_ps(floatVal);
  
  ezSimdVec4d result;
  result.m_v.xy = _mm_cvtps_pd(floatRcp);
  result.m_v.zw = result.m_v.xy;

    //TODO optimize further?
  return result;
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal<ezMathAcc::BITS_23>() const
{
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
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal<ezMathAcc::FULL>() const
{
  ezSimdVec4d result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
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
  ezSimdVec4d result;
  result.m_v.xy = _mm_sqrt_pd(m_v.xy);
  result.m_v.zw = result.m_v.xy;
  return result;
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt<ezMathAcc::FULL>() const
{
  ezSimdVec4d result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, _mm_sqrt_pd(m_v.xy));
  result.m_v.zw = result.m_v.xy;
  return result;
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt<ezMathAcc::BITS_23>() const
{
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
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt<ezMathAcc::BITS_12>() const
{
  // Convert double to float, get fast inverse sqrt approximation, convert back to double
  __m128 floatVal = _mm_cvtpd_ps(m_v.xy);
  __m128 floatInvSqrt = _mm_rsqrt_ps(floatVal);
  
  ezSimdVec4d result;
  result.m_v.xy = _mm_cvtps_pd(floatInvSqrt);
  result.m_v.zw = result.m_v.xy;
  return result;
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
  ezSimdVec4d result;
  __m128d zero = _mm_setzero_pd();
  result.m_v.xy = _mm_sub_pd(zero, m_v.xy);
  result.m_v.zw = _mm_sub_pd(zero, m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator+(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = _mm_add_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_add_pd(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator-(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = _mm_sub_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_sub_pd(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator*(const ezSimdDouble& f) const
{
  ezSimdVec4d result;
  result.m_v.xy = _mm_mul_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = _mm_mul_pd(m_v.zw, f.m_v.xy);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator/(const ezSimdDouble& f) const
{
  ezSimdVec4d result;
  result.m_v.xy = _mm_div_pd(m_v.xy, f.m_v.xy);
  result.m_v.zw = _mm_div_pd(m_v.zw, f.m_v.xy);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMul(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = _mm_mul_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_mul_pd(m_v.zw, v.m_v.zw);
  return result;
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompDiv<ezMathAcc::FULL>(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = _mm_div_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_div_pd(m_v.zw, v.m_v.zw);
  return result;
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
  ezSimdVec4d result;
  result.m_v.xy = _mm_min_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_min_pd(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMax(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = _mm_max_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_max_pd(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Abs() const
{
  ezSimdVec4d result;
  __m128d signMask = _mm_set1_pd(-0.0);
  result.m_v.xy = _mm_andnot_pd(signMask, m_v.xy);
  result.m_v.zw = _mm_andnot_pd(signMask, m_v.zw);
  return result;
}