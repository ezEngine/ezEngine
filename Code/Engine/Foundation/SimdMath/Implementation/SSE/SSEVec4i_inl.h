#pragma once

#if EZ_ENABLED(EZ_COMPILER_MSVC)
#  include <intrin.h>
#endif

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 iXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_epi32(iXyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInternal::QuadInt v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::MakeZero()
{
  return _mm_setzero_si128();
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 iXyzw)
{
  m_v = _mm_set1_epi32(iXyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::SetZero()
{
  m_v = _mm_setzero_si128();
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<1>(const ezInt32* pInts)
{
  m_v = _mm_loadu_si32(pInts);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<2>(const ezInt32* pInts)
{
  m_v = _mm_loadu_si64(pInts);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<3>(const ezInt32* pInts)
{
  m_v = _mm_setr_epi32(pInts[0], pInts[1], pInts[2], 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<4>(const ezInt32* pInts)
{
  m_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pInts));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<1>(ezInt32* pInts) const
{
  _mm_storeu_si32(pInts, m_v);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<2>(ezInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<3>(ezInt32* pInts) const
{
  _mm_storeu_si64(pInts, m_v);
  _mm_storeu_si32(pInts + 2, _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(m_v), _mm_castsi128_ps(m_v))));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<4>(ezInt32* pInts) const
{
  _mm_storeu_si128(reinterpret_cast<__m128i*>(pInts), m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4i::ToFloat() const
{
  return _mm_cvtepi32_ps(m_v);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Truncate(const ezSimdVec4f& f)
{
  return _mm_cvttps_epi32(f.m_v);
}

template <int N>
EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::GetComponent() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return ((ezInt32*)&m_v)[N];
  // return m_v.m128i_i32[N];
#endif
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::x() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::y() const
{
  return GetComponent<1>();
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::z() const
{
  return GetComponent<2>();
}

EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::w() const
{
  return GetComponent<3>();
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Get() const
{
  return _mm_shuffle_epi32(m_v, EZ_TO_SHUFFLE(s));
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::GetCombined(const ezSimdVec4i& other) const
{
  return _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(m_v), _mm_castsi128_ps(other.m_v), EZ_TO_SHUFFLE(s)));
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator-() const
{
  return _mm_sub_epi32(_mm_setzero_si128(), m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator+(const ezSimdVec4i& v) const
{
  return _mm_add_epi32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator-(const ezSimdVec4i& v) const
{
  return _mm_sub_epi32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMul(const ezSimdVec4i& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_mullo_epi32(m_v, v.m_v);
#else
  __m128i tmp1 = _mm_mul_epu32(m_v, v.m_v);
  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(m_v, 4), _mm_srli_si128(v.m_v, 4));
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, EZ_SHUFFLE(0, 2, 0, 0)), _mm_shuffle_epi32(tmp2, EZ_SHUFFLE(0, 2, 0, 0)));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompDiv(const ezSimdVec4i& v) const
{
#if EZ_ENABLED(EZ_COMPILER_MSVC)
  return _mm_div_epi32(m_v, v.m_v);
#else
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] / b[i];
  }

  ezSimdVec4i r;
  r.Load<4>(a);
  return r;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator|(const ezSimdVec4i& v) const
{
  return _mm_or_si128(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator&(const ezSimdVec4i& v) const
{
  return _mm_and_si128(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator^(const ezSimdVec4i& v) const
{
  return _mm_xor_si128(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator~() const
{
  __m128i ones = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_xor_si128(ones, m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator<<(ezUInt32 uiShift) const
{
  return _mm_slli_epi32(m_v, uiShift);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator>>(ezUInt32 uiShift) const
{
  return _mm_srai_epi32(m_v, uiShift);
}

EZ_FORCE_INLINE ezSimdVec4i ezSimdVec4i::operator<<(const ezSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] << b[i];
  }

  ezSimdVec4i r;
  r.Load<4>(a);
  return r;
}

EZ_FORCE_INLINE ezSimdVec4i ezSimdVec4i::operator>>(const ezSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] >> b[i];
  }

  ezSimdVec4i r;
  r.Load<4>(a);
  return r;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator+=(const ezSimdVec4i& v)
{
  m_v = _mm_add_epi32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator-=(const ezSimdVec4i& v)
{
  m_v = _mm_sub_epi32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator|=(const ezSimdVec4i& v)
{
  m_v = _mm_or_si128(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator&=(const ezSimdVec4i& v)
{
  m_v = _mm_and_si128(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator^=(const ezSimdVec4i& v)
{
  m_v = _mm_xor_si128(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator<<=(ezUInt32 uiShift)
{
  m_v = _mm_slli_epi32(m_v, uiShift);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator>>=(ezUInt32 uiShift)
{
  m_v = _mm_srai_epi32(m_v, uiShift);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMin(const ezSimdVec4i& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_min_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmplt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMax(const ezSimdVec4i& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_max_epi32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmpgt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Abs() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_31
  return _mm_abs_epi32(m_v);
#else
  __m128i negMask = _mm_cmplt_epi32(m_v, _mm_setzero_si128());
  __m128i neg = _mm_sub_epi32(_mm_setzero_si128(), m_v);
  return _mm_or_si128(_mm_and_si128(negMask, neg), _mm_andnot_si128(negMask, m_v));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator==(const ezSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpeq_epi32(m_v, v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator!=(const ezSimdVec4i& v) const
{
  return !(*this == v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator<=(const ezSimdVec4i& v) const
{
  return !(*this > v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator<(const ezSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmplt_epi32(m_v, v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator>=(const ezSimdVec4i& v) const
{
  return !(*this < v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator>(const ezSimdVec4i& v) const
{
  return _mm_castsi128_ps(_mm_cmpgt_epi32(m_v, v.m_v));
}

// static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Select(const ezSimdVec4b& vCmp, const ezSimdVec4i& vTrue, const ezSimdVec4i& vFalse)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(vFalse.m_v), _mm_castsi128_ps(vTrue.m_v), vCmp.m_v));
#else
  return _mm_castps_si128(_mm_or_ps(_mm_andnot_ps(vCmp.m_v, _mm_castsi128_ps(vFalse.m_v)), _mm_and_ps(vCmp.m_v, _mm_castsi128_ps(vTrue.m_v))));
#endif
}

// not needed atm
#if 0
void ezSimdVec4i::Transpose(ezSimdVec4i& v0, ezSimdVec4i& v1, ezSimdVec4i& v2, ezSimdVec4i& v3)
{
  __m128i T0 = _mm_unpacklo_epi32(v0.m_v, v1.m_v);
  __m128i T1 = _mm_unpacklo_epi32(v2.m_v, v3.m_v);
  __m128i T2 = _mm_unpackhi_epi32(v0.m_v, v1.m_v);
  __m128i T3 = _mm_unpackhi_epi32(v2.m_v, v3.m_v);

  v0.m_v = _mm_unpacklo_epi64(T0, T1);
  v1.m_v = _mm_unpackhi_epi64(T0, T1);
  v2.m_v = _mm_unpacklo_epi64(T2, T3);
  v3.m_v = _mm_unpackhi_epi64(T2, T3);
}
#endif
