#pragma once

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i()
{
  EZ_CHECK_ALIGNMENT_16(this);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 xyzw)
{
  EZ_CHECK_ALIGNMENT_16(this);

  m_v = _mm_set1_epi32(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  EZ_CHECK_ALIGNMENT_16(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInternal::QuadInt v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 xyzw)
{
  m_v = _mm_set1_epi32(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::SetZero()
{
  m_v = _mm_setzero_si128();
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4i::ToFloat() const
{
  return _mm_cvtepi32_ps(m_v);
}

//static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Truncate(const ezSimdVec4f& f)
{
  return _mm_cvttps_epi32(f.m_v);
}

template<int N>
EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::GetComponent() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return m_v.m128i_i32[N];
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
  const int shuffle = ((s >> 12) & 0x03) | ((s >> 6) & 0x0c) | (s & 0x30) | ((s << 6) & 0xc0);
  return _mm_shuffle_epi32(m_v, shuffle);
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
  EZ_ASSERT_NOT_IMPLEMENTED; // not sure whether this code works so better assert
  __m128i tmp1 = _mm_mul_epu32(m_v, v.m_v);
  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(m_v, 4), _mm_srli_si128(v.m_v, 4));
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, _MM_SHUFFLE(0, 0, 2, 0)), _mm_shuffle_epi32(tmp2, _MM_SHUFFLE(0, 0, 2, 0)));
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
  __m128i negMask = _mm_cmplt_epi32(m_v, _mm_setzero_si128());
  __m128i neg = _mm_sub_epi32(_mm_setzero_si128(), m_v);
  return _mm_or_si128(_mm_and_si128(negMask, neg), _mm_andnot_si128(negMask, m_v));
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

//static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::ZeroVector()
{
  return _mm_setzero_si128();
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
