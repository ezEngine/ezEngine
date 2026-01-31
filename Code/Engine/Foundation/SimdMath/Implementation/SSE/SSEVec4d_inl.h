#pragma once

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_set1_pd(static_cast<double>(fXyzw));
#else
  m_v.xy = _mm_set1_pd(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_set1_pd(fXyzw);
#else
  m_v.xy = _mm_set1_pd(fXyzw);
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(const ezSimdDouble& fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = fXyzw.m_v;
}
EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(int x, int y, int z, int w)
{
    EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setr_pd(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w));
#else
  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
#endif

}
EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float x, float y, float z, float w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setr_pd(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w));
#else
  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double x, double y, double z, double w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setr_pd(x, y, z, w);
#else
  m_v.xy = _mm_setr_pd(x, y);
  m_v.zw = _mm_setr_pd(z, w);
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float fXyzw)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_set1_pd(static_cast<double>(fXyzw));
#else
  m_v.xy = _mm_set1_pd(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double fXyzw)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_set1_pd(fXyzw);
#else
  m_v.xy = _mm_set1_pd(fXyzw);
  m_v.zw = m_v.xy;
#endif
}
EZ_ALWAYS_INLINE void ezSimdVec4d::Set(int x, int y, int z, int w)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setr_pd(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w));
#else
  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float x, float y, float z, float w)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setr_pd(static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w));
#else
  m_v.xy = _mm_setr_pd(static_cast<double>(x), static_cast<double>(y));
  m_v.zw = _mm_setr_pd(static_cast<double>(z), static_cast<double>(w));
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double x, double y, double z, double w)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setr_pd(x, y, z, w);
#else
  m_v.xy = _mm_setr_pd(x, y);
  m_v.zw = _mm_setr_pd(z, w);
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetX(const ezSimdDouble& f)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x1);
#else
m_v.xy = _mm_shuffle_pd(f.m_v.xy, m_v.xy, EZ_SHUFFLE_2(0, 1));
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetY(const ezSimdDouble& f)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x2);
#else
m_v.xy = _mm_shuffle_pd(m_v.xy, f.m_v.xy, EZ_SHUFFLE_2(0, 1));
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZ(const ezSimdDouble& f)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x4);
#else
m_v.zw = _mm_shuffle_pd(f.m_v.zw, m_v.zw, EZ_SHUFFLE_2(0, 1));
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetW(const ezSimdDouble& f)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_blend_pd(m_v, f.m_v, 0x8);
#else
m_v.zw = _mm_shuffle_pd(m_v.zw, f.m_v.zw, EZ_SHUFFLE_2(0, 1));
#endif
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZero()
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setzero_pd();
#else
  m_v.xy = _mm_setzero_pd();
  m_v.zw = m_v.xy;
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<1>(const float* pFloat)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128 temp = _mm_load_ss(pFloat);
  m_v = _mm256_cvtps_pd(temp);
#else
  m_v.xy = _mm_cvtps_pd(_mm_load_ss(pFloat));
  m_v.zw = _mm_setzero_pd();
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<2>(const float* pFloat)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128 temp = _mm_setr_ps(pFloat[0], pFloat[1], 0.0f, 0.0f);
  m_v = _mm256_cvtps_pd(temp);
#else
  __m128 temp = _mm_setr_ps(pFloat[0], pFloat[1], 0.0f, 0.0f);
  m_v.xy = _mm_cvtps_pd(temp);
  m_v.zw = _mm_setzero_pd();
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<3>(const float* pFloat)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128 temp = _mm_set_ps(0.0f, pFloat[2], pFloat[1], pFloat[0]);
  m_v = _mm256_cvtps_pd(temp);
#else
  __m128 temp = _mm_set_ps(0.0f, pFloat[2], pFloat[1], pFloat[0]);
  m_v.xy = _mm_cvtps_pd(temp);
  m_v.zw = _mm_cvtps_pd(_mm_movehl_ps(temp, temp));
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<4>(const float* pFloat)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_cvtps_pd(_mm_loadu_ps(pFloat));
#else
  __m128 temp = _mm_loadu_ps(pFloat);
  m_v.xy = _mm_cvtps_pd(temp);
  m_v.zw = _mm_cvtps_pd(_mm_movehl_ps(temp, temp));
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<1>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128d val = _mm_load_sd(pDouble);
  m_v = _mm256_insertf128_pd(_mm256_castpd128_pd256(val), _mm_setzero_pd(), 1);
#else
  m_v.xy = _mm_load_sd(pDouble);
  m_v.zw = _mm_setzero_pd();
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<2>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128d vals = _mm_loadu_pd(pDouble);
  m_v = _mm256_insertf128_pd(_mm256_castpd128_pd256(vals), _mm_setzero_pd(), 1);
#else
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = _mm_setzero_pd();
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<3>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_setr_pd(pDouble[0], pDouble[1], pDouble[2], 0.0);
#else
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = _mm_load_sd(pDouble + 2);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<4>(const double* pDouble)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_loadu_pd(pDouble);
#else
  m_v.xy = _mm_loadu_pd(pDouble);
  m_v.zw = _mm_loadu_pd(pDouble + 2);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<1>(float* pFloat) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128 temp = _mm256_cvtpd_ps(m_v);
  _mm_store_ss(pFloat, temp);
#else
  __m128 temp = _mm_cvtpd_ps(m_v.xy);
  _mm_store_ss(pFloat, temp);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<2>(float* pFloat) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128 temp = _mm256_cvtpd_ps(m_v);
  _mm_store_ss(pFloat, temp);
  _mm_store_ss(pFloat + 1, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 1, 1, 1)));
#else
  __m128 temp = _mm_cvtpd_ps(m_v.xy);
  _mm_store_ss(pFloat, temp);
  _mm_store_ss(pFloat + 1, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 1, 1, 1)));
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<3>(float* pFloat) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128 temp = _mm256_cvtpd_ps(m_v);
  _mm_store_ss(pFloat, temp);
  _mm_store_ss(pFloat + 1, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(1, 1, 1, 1)));
  _mm_store_ss(pFloat + 2, _mm_shuffle_ps(temp, temp, _MM_SHUFFLE(2, 2, 2, 2)));
#else
  __m128 temp_xy = _mm_cvtpd_ps(m_v.xy);
  __m128 temp_zw = _mm_cvtpd_ps(m_v.zw);
  _mm_store_ss(pFloat, temp_xy);
  _mm_store_ss(pFloat + 1, _mm_shuffle_ps(temp_xy, temp_xy, _MM_SHUFFLE(1, 1, 1, 1)));
  _mm_store_ss(pFloat + 2, temp_zw);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<4>(float* pFloat) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  _mm_storeu_ps(pFloat, _mm256_cvtpd_ps(m_v));
#else
  __m128 temp_xy = _mm_cvtpd_ps(m_v.xy);
  __m128 temp_zw = _mm_cvtpd_ps(m_v.zw);
  __m128 result = _mm_movelh_ps(temp_xy, temp_zw);
  _mm_storeu_ps(pFloat, result);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<1>(double* pDouble) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  _mm_store_sd(pDouble, _mm256_castpd256_pd128(m_v));
#else
  _mm_store_sd(pDouble, m_v.xy);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<2>(double* pDouble) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  _mm_storeu_pd(pDouble, _mm256_castpd256_pd128(m_v));
#else
  _mm_storeu_pd(pDouble, m_v.xy);
#endif
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<3>(double* pDouble) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  _mm256_storeu_pd(pDouble, m_v);
#else
  _mm_storeu_pd(pDouble, m_v.xy);
  _mm_storeu_pd(pDouble + 2, m_v.zw);
#endif
}



EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_div_pd(_mm256_set1_pd(1.0), m_v);
#else
  ezSimdVec4d result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, m_v.xy);
  result.m_v.zw = _mm_div_pd(one, m_v.zw);
  return result;
#endif
}


EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetSqrt() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_sqrt_pd(m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_sqrt_pd(m_v.xy);
  result.m_v.zw = _mm_sqrt_pd(m_v.zw);
  return result;
#endif
}


EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_div_pd(_mm256_set1_pd(1.0), _mm256_sqrt_pd(m_v));
#else
  ezSimdVec4d result;
  __m128d one = _mm_set1_pd(1.0);
  result.m_v.xy = _mm_div_pd(one, _mm_sqrt_pd(m_v.xy));
  result.m_v.zw = _mm_div_pd(one, _mm_sqrt_pd(m_v.zw));
  return result;
#endif
}


template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetComponent() const
{
  ezSimdDouble result;
#if EZ_SSE_LEVEL >= EZ_SSE_AVX

  const int lane_selector = 0x11 * (N >> 1); 
  const int permute_mask = 0xF * (N & 1);     

  result.m_v = _mm256_permute2f128_pd(m_v, m_v, lane_selector);
  result.m_v = _mm256_permute_pd(result.m_v, permute_mask);

  return result;
#else

  if constexpr (N < 2)
  {
    //broadcast from 0 or 1 to result
    result.m_v.xy = _mm_shuffle_pd(m_v.xy, m_v.xy, EZ_SHUFFLE_2(N,N));
    result.m_v.zw = result.m_v.xy;
  }
  else
  {
    //broadcast from 2 or 3 to result
    result.m_v.xy = _mm_shuffle_pd(m_v.zw, m_v.zw, EZ_SHUFFLE_2(N-2,N-2));
    result.m_v.zw = result.m_v.xy;
  }
  return result;

#endif

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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_mul_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_mul_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_mul_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}


EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompDiv(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_div_pd(m_v, v.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_div_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_div_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}



EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMin(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
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
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_andnot_pd(_mm256_set1_pd(-0.0), m_v);
#else
  ezSimdVec4d result;
  __m128d signMask = _mm_set1_pd(-0.0);
  result.m_v.xy = _mm_andnot_pd(signMask, m_v.xy);
  result.m_v.zw = _mm_andnot_pd(signMask, m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Round() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_NINT);
#else
  return ezSimdVec4d(floor(static_cast<double>(x()) + 0.5), floor(static_cast<double>(y()) + 0.5), floor(static_cast<double>(z()) + 0.5), floor(static_cast<double>(w()) + 0.5));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Floor() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_FLOOR);
#else
  return ezSimdVec4d(floor(static_cast<double>(x())), floor(static_cast<double>(y())), floor(static_cast<double>(z())), floor(static_cast<double>(w())));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Ceil() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_CEIL);
#else
  return ezSimdVec4d(ceil(static_cast<double>(x())), ceil(static_cast<double>(y())), ceil(static_cast<double>(z())), ceil(static_cast<double>(w())));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Trunc() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_round_pd(m_v, _MM_FROUND_TRUNC);
#else
  return ezSimdVec4d(double(int(x())), double(int(y())), double(int(z())), double(int(w())));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::FlipSign(const ezSimdVec4bWide& vCmp) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_xor_pd(m_v, _mm256_and_pd(vCmp.m_v, _mm256_set1_pd(-0.0)));
#else
  ezSimdVec4d result;

  result.m_v.xy = _mm_xor_pd(m_v.xy, _mm_and_pd(vCmp.m_v.xy, _mm_set1_pd(-0.0)));
  result.m_v.zw = _mm_xor_pd(m_v.zw, _mm_and_pd(vCmp.m_v.zw, _mm_set1_pd(-0.0)));
  return result;
#endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Select(const ezSimdVec4bWide& vCmp, const ezSimdVec4d& vTrue, const ezSimdVec4d& vFalse)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_blendv_pd(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  ezSimdVec4d result;
  result.m_v.xy = _mm_or_pd(_mm_andnot_pd(vCmp.m_v.xy, vFalse.m_v.xy), _mm_and_pd(vCmp.m_v.xy, vTrue.m_v.xy));
  result.m_v.zw = _mm_or_pd(_mm_andnot_pd(vCmp.m_v.zw, vFalse.m_v.zw), _mm_and_pd(vCmp.m_v.zw, vTrue.m_v.zw));
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator+=(const ezSimdVec4d& v)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_add_pd(m_v, v.m_v);
#else
  m_v.xy = _mm_add_pd(m_v.xy, v.m_v.xy);
  m_v.zw = _mm_add_pd(m_v.zw, v.m_v.zw);
#endif
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator-=(const ezSimdVec4d& v)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_sub_pd(m_v, v.m_v);
#else
  m_v.xy = _mm_sub_pd(m_v.xy, v.m_v.xy);
  m_v.zw = _mm_sub_pd(m_v.zw, v.m_v.zw);
#endif
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator*=(const ezSimdDouble& f)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_mul_pd(m_v, f.m_v);
#else
  m_v.xy = _mm_mul_pd(m_v.xy, f.m_v.xy);
  m_v.zw = _mm_mul_pd(m_v.zw, f.m_v.xy);
#endif
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator/=(const ezSimdDouble& f)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  m_v = _mm256_div_pd(m_v, f.m_v);
#else
  m_v.xy = _mm_div_pd(m_v.xy, f.m_v.xy);
  m_v.zw = _mm_div_pd(m_v.zw, f.m_v.xy);
#endif
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator==(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4bWide result;
  result.m_v = _mm256_cmp_pd(m_v, v.m_v, _CMP_EQ_OQ);
  return result;
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_cmpeq_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_cmpeq_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator!=(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4bWide result;
  result.m_v = _mm256_cmp_pd(m_v, v.m_v, _CMP_NEQ_OQ);
  return result;
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_cmpneq_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_cmpneq_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator<=(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4bWide result;
  result.m_v = _mm256_cmp_pd(m_v, v.m_v, _CMP_LE_OQ);
  return result;
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_cmple_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_cmple_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator<(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4bWide result;
  result.m_v = _mm256_cmp_pd(m_v, v.m_v, _CMP_LT_OQ);
  return result;
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_cmplt_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_cmplt_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator>=(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4bWide result;
  result.m_v = _mm256_cmp_pd(m_v, v.m_v, _CMP_GE_OQ);
  return result;
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_cmpge_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_cmpge_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator>(const ezSimdVec4d& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4bWide result;
  result.m_v = _mm256_cmp_pd(m_v, v.m_v, _CMP_GT_OQ);
  return result;
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_cmpgt_pd(m_v.xy, v.m_v.xy);
  result.m_v.zw = _mm_cmpgt_pd(m_v.zw, v.m_v.zw);
  return result;
#endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulAdd(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm256_fmadd_pd(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) + c;
#endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulAdd(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm256_fmadd_pd(a.m_v, b.m_v, c.m_v);
#else
  return a * b + c;
#endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulSub(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm256_fmsub_pd(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) - c;
#endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulSub(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm256_fmsub_pd(a.m_v, b.m_v, c.m_v);
#else
  return a * b - c;
#endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CopySign(const ezSimdVec4d& vMagnitude, const ezSimdVec4d& vSign)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m256d minusZero = _mm256_set1_pd(-0.0);
  return _mm256_or_pd(_mm256_andnot_pd(minusZero, vMagnitude.m_v), _mm256_and_pd(minusZero, vSign.m_v));
#else
  ezSimdVec4d result;
  __m128d minusZero = _mm_set1_pd(-0.0);
  result.m_v.xy = _mm_or_pd(_mm_andnot_pd(minusZero, vMagnitude.m_v.xy), _mm_and_pd(minusZero, vSign.m_v.xy));
  result.m_v.zw = _mm_or_pd(_mm_andnot_pd(minusZero, vMagnitude.m_v.zw), _mm_and_pd(minusZero, vSign.m_v.zw));
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CrossRH(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  double x1 = static_cast<double>(x());
  double y1 = static_cast<double>(y());
  double z1 = static_cast<double>(z());
  double x2 = static_cast<double>(v.x());
  double y2 = static_cast<double>(v.y());
  double z2 = static_cast<double>(v.z());
  
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m256d temp = _mm256_setr_pd(y1 * z2 - z1 * y2, z1 * x2 - x1 * z2, x1 * y2 - y1 * x2, 0.0);
  result.m_v = temp;
#else
  result.m_v.xy = _mm_setr_pd(y1 * z2 - z1 * y2, z1 * x2 - x1 * z2);
  result.m_v.zw = _mm_setr_pd(x1 * y2 - y1 * x2, 0.0);
#endif
  return result;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<2>() const
{

  return GetComponent<0>() + GetComponent<1>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<4>() const
{

  return (GetComponent<0>() + GetComponent<1>()) + (GetComponent<2>() + GetComponent<3>());

}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<2>() const
{
  ezSimdDouble c0 = GetComponent<0>();
  ezSimdDouble c1 = GetComponent<1>();
  return c0.Min(c1);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<3>() const
{
  ezSimdDouble c0 = GetComponent<0>();
  ezSimdDouble c1 = GetComponent<1>();
  ezSimdDouble c2 = GetComponent<2>();
  return c0.Min(c1).Min(c2);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<4>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128d lo = _mm256_castpd256_pd128(m_v);
  __m128d hi = _mm256_extractf128_pd(m_v, 1);
  __m128d minXYZW = _mm_min_pd(lo, hi);
  __m128d minYXWZ = _mm_shuffle_pd(minXYZW, minXYZW, _MM_SHUFFLE2(0, 1));
  __m128d minResult = _mm_min_pd(minXYZW, minYXWZ);
  
  ezSimdDouble result;
  result.m_v = _mm256_insertf128_pd(_mm256_castpd128_pd256(minResult), minResult, 1);
  return result;
#else
  return GetComponent<0>().Min(GetComponent<1>()).Min(GetComponent<2>()).Min(GetComponent<3>());
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<2>() const
{
  ezSimdDouble c0 = GetComponent<0>();
  ezSimdDouble c1 = GetComponent<1>();
  return c0.Max(c1);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<3>() const
{
  ezSimdDouble c0 = GetComponent<0>();
  ezSimdDouble c1 = GetComponent<1>();
  ezSimdDouble c2 = GetComponent<2>();
  return c0.Max(c1).Max(c2);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<4>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m128d lo = _mm256_castpd256_pd128(m_v);
  __m128d hi = _mm256_extractf128_pd(m_v, 1);
  __m128d maxXYZW = _mm_max_pd(lo, hi);
  __m128d maxYXWZ = _mm_shuffle_pd(maxXYZW, maxXYZW, _MM_SHUFFLE2(0, 1));
  __m128d maxResult = _mm_max_pd(maxXYZW, maxYXWZ);
  
  ezSimdDouble result;
  result.m_v = _mm256_insertf128_pd(_mm256_castpd128_pd256(maxResult), maxResult, 1);
  return result;
#else
  return GetComponent<0>().Max(GetComponent<1>()).Max(GetComponent<2>()).Max(GetComponent<3>());
#endif
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<1>(const ezSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<1>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<2>(const ezSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<2>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<3>(const ezSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<3>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<4>(const ezSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<4>();
}


template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Get() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4d result;
  EZ_WIDE_SWIZZLE_AVX1(m_v, s, result.m_v);
  return result;
#else
  ezSimdVec4d result;
  EZ_WIDE_SHUFFLE_SSE(m_v.xy, m_v.zw, m_v.xy, m_v.zw, EZ_TO_SHUFFLE(s), result.m_v.xy, result.m_v.zw);
  return result;

#endif
}


template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsZero() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  const int mask = EZ_BIT(N) - 1;
  __m256d zero = _mm256_setzero_pd();
  __m256d cmp = _mm256_cmp_pd(m_v, zero, _CMP_EQ_OQ);
  return (_mm256_movemask_pd(cmp) & mask) == mask;
#else
  const int mask = EZ_BIT(N) - 1;
  int m1 = _mm_movemask_pd(_mm_cmpeq_pd(m_v.xy, _mm_setzero_pd()));
  int m2 = _mm_movemask_pd(_mm_cmpeq_pd(m_v.zw, _mm_setzero_pd()));
  return ((m1 | (m2 << 2)) & mask) == mask;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsZero(const ezSimdDouble& fEpsilon) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  const int mask = EZ_BIT(N) - 1;
  __m256d absVal = _mm256_andnot_pd(_mm256_set1_pd(-0.0), m_v);
  __m256d cmp = _mm256_cmp_pd(absVal, fEpsilon.m_v, _CMP_LT_OQ);
  return (_mm256_movemask_pd(cmp) & mask) == mask;
#else
  const int mask = EZ_BIT(N) - 1;
  __m128d signMask = _mm_set1_pd(-0.0);
  __m128d absXY = _mm_andnot_pd(signMask, m_v.xy);
  __m128d absZW = _mm_andnot_pd(signMask, m_v.zw);
  int m1 = _mm_movemask_pd(_mm_cmplt_pd(absXY, fEpsilon.m_v.xy));
  int m2 = _mm_movemask_pd(_mm_cmplt_pd(absZW, fEpsilon.m_v.xy));
  return ((m1 | (m2 << 2)) & mask) == mask;
#endif
}

template <int N>
inline bool ezSimdVec4d::IsNaN() const
{
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // For double: exponent = 11 bits, sign bit mask = 0x7FF0000000000000
  
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  alignas(32) const ezUInt64 s_exponentMask[4] = {0x7FF0000000000000ULL, 0x7FF0000000000000ULL, 0x7FF0000000000000ULL, 0x7FF0000000000000ULL};
  alignas(32) const ezUInt64 s_mantissaMask[4] = {0x000FFFFFFFFFFFFFULL, 0x000FFFFFFFFFFFFFULL, 0x000FFFFFFFFFFFFFULL, 0x000FFFFFFFFFFFFFULL};

  __m256d exponentMask = _mm256_load_pd(reinterpret_cast<const double*>(s_exponentMask));
  __m256d mantissaMask = _mm256_load_pd(reinterpret_cast<const double*>(s_mantissaMask));

  __m256d exponentAll1 = _mm256_cmp_pd(_mm256_and_pd(m_v, exponentMask), exponentMask, _CMP_EQ_OQ);
  __m256d mantissaNon0 = _mm256_cmp_pd(_mm256_and_pd(m_v, mantissaMask), _mm256_setzero_pd(), _CMP_NEQ_OQ);

  const int mask = EZ_BIT(N) - 1;
  return (_mm256_movemask_pd(_mm256_and_pd(exponentAll1, mantissaNon0)) & mask) != 0;
#else
  alignas(16) const ezUInt64 s_exponentMask[2] = {0x7FF0000000000000ULL, 0x7FF0000000000000ULL};
  alignas(16) const ezUInt64 s_mantissaMask[2] = {0x000FFFFFFFFFFFFFULL, 0x000FFFFFFFFFFFFFULL};

  __m128d exponentMask = _mm_load_pd(reinterpret_cast<const double*>(s_exponentMask));
  __m128d mantissaMask = _mm_load_pd(reinterpret_cast<const double*>(s_mantissaMask));

  __m128d exponentAll1_xy = _mm_cmpeq_pd(_mm_and_pd(m_v.xy, exponentMask), exponentMask);
  __m128d mantissaNon0_xy = _mm_cmpneq_pd(_mm_and_pd(m_v.xy, mantissaMask), _mm_setzero_pd());
  __m128d exponentAll1_zw = _mm_cmpeq_pd(_mm_and_pd(m_v.zw, exponentMask), exponentMask);
  __m128d mantissaNon0_zw = _mm_cmpneq_pd(_mm_and_pd(m_v.zw, mantissaMask), _mm_setzero_pd());

  int m1 = _mm_movemask_pd(_mm_and_pd(exponentAll1_xy, mantissaNon0_xy));
  int m2 = _mm_movemask_pd(_mm_and_pd(exponentAll1_zw, mantissaNon0_zw));
  
  const int mask = EZ_BIT(N) - 1;
  return ((m1 | (m2 << 2)) & mask) != 0;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsValid() const
{
  // Check the 11 exponent bits for double
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  alignas(32) const ezUInt64 s_exponentMask[4] = {0x7FF0000000000000ULL, 0x7FF0000000000000ULL, 0x7FF0000000000000ULL, 0x7FF0000000000000ULL};

  __m256d exponentMask = _mm256_load_pd(reinterpret_cast<const double*>(s_exponentMask));
  __m256d exponentNot1 = _mm256_cmp_pd(_mm256_and_pd(m_v, exponentMask), exponentMask, _CMP_NEQ_OQ);

  const int mask = EZ_BIT(N) - 1;
  return (_mm256_movemask_pd(exponentNot1) & mask) == mask;
#else
  alignas(16) const ezUInt64 s_exponentMask[2] = {0x7FF0000000000000ULL, 0x7FF0000000000000ULL};

  __m128d exponentMask = _mm_load_pd(reinterpret_cast<const double*>(s_exponentMask));

  __m128d exponentNot1_xy = _mm_cmpneq_pd(_mm_and_pd(m_v.xy, exponentMask), exponentMask);
  __m128d exponentNot1_zw = _mm_cmpneq_pd(_mm_and_pd(m_v.zw, exponentMask), exponentMask);

  int m1 = _mm_movemask_pd(exponentNot1_xy);
  int m2 = _mm_movemask_pd(exponentNot1_zw);
  
  const int mask = EZ_BIT(N) - 1;
  return ((m1 | (m2 << 2)) & mask) == mask;
#endif
}

template <int N>
void ezSimdVec4d::NormalizeIfNotZero(const ezSimdDouble& fEpsilon)
{
  ezSimdDouble sqLength = GetLengthSquared<N>();

#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m256d isNotZero = _mm256_cmp_pd(sqLength.m_v, fEpsilon.m_v, _CMP_GT_OQ);
  __m256d invSqrt = sqLength.GetInvSqrt().m_v;
  m_v = _mm256_and_pd(_mm256_mul_pd(m_v, invSqrt), isNotZero);
#else
  __m128d isNotZero = _mm_cmpgt_pd(sqLength.m_v.xy, fEpsilon.m_v.xy);
  __m128d invSqrt = sqLength.GetInvSqrt().m_v.xy;
  m_v.xy = _mm_and_pd(_mm_mul_pd(m_v.xy, invSqrt), isNotZero);
  m_v.zw = _mm_and_pd(_mm_mul_pd(m_v.zw, invSqrt), isNotZero);
#endif
}

  ///\brief x = this[s0], y = this[s1], z = other[s2], w = other[s3]
template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetCombined(const ezSimdVec4d& other) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4d result;
  EZ_WIDE_SHUFFLE_AVX1(m_v, other.m_v, EZ_TO_SHUFFLE(s), result.m_v);
  return result;
#else
  ezSimdVec4d result;
  EZ_WIDE_SHUFFLE_SSE(m_v.xy, m_v.zw, other.m_v.xy, other.m_v.zw, EZ_TO_SHUFFLE(s), result.m_v.xy, result.m_v.zw);
  return result;

#endif
}