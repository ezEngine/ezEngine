#pragma once

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  m_v = _mm_set1_epi32(0xCDCDCDCD);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezUInt32 uiXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_set1_epi32(uiXyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = _mm_setr_epi32(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezInternal::QuadInt v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE void ezSimdVec4u::Set(ezUInt32 uiXyzw)
{
  m_v = _mm_set1_epi32(uiXyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4u::Set(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w)
{
  m_v = _mm_setr_epi32(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4u::SetZero()
{
  m_v = _mm_setzero_si128();
}

// needs to be implemented here because of include dependencies
EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(const ezSimdVec4u& u)
  : m_v(u.m_v)
{
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(const ezSimdVec4i& i)
  : m_v(i.m_v)
{
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4u::ToFloat() const
{
  __m128 two16 = _mm_set1_ps((float)0x10000); // 2^16
  __m128i high = _mm_srli_epi32(m_v, 16);
  __m128i low = _mm_srli_epi32(_mm_slli_epi32(m_v, 16), 16);
  __m128 fHigh = _mm_mul_ps(_mm_cvtepi32_ps(high), two16);
  __m128 fLow = _mm_cvtepi32_ps(low);

  return _mm_add_ps(fHigh, fLow);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::Truncate(const ezSimdVec4f& f)
{
  alignas(16) const float fmax[4] = {2.14748364e+009f, 2.14748364e+009f, 2.14748364e+009f, 2.14748364e+009f};
  alignas(16) const float fmax_unsigned[4] = {4.29496729e+009f, 4.29496729e+009f, 4.29496729e+009f, 4.29496729e+009f};
  __m128i zero = _mm_setzero_si128();
  __m128i mask = _mm_cmpgt_epi32(_mm_castps_si128(f.m_v), zero);
  __m128 min = _mm_and_ps(_mm_castsi128_ps(mask), f.m_v);
  __m128 max = _mm_min_ps(min, _mm_load_ps(fmax_unsigned)); // clamped in 0 - 4.29496729+009

  __m128 diff = _mm_sub_ps(max, _mm_load_ps(fmax));
  mask = _mm_cmpgt_epi32(_mm_castps_si128(diff), zero);
  diff = _mm_and_ps(_mm_castsi128_ps(mask), diff);

  __m128i res1 = _mm_cvttps_epi32(diff);
  __m128i res2 = _mm_cvttps_epi32(max);
  return _mm_add_epi32(res1, res2);
}

template <int N>
EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::GetComponent() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_extract_epi32(m_v, N);
#else
  return ((ezUInt32*)&m_v)[N];
  //return m_v.m128i_i32[N];
#endif
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::x() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::y() const
{
  return GetComponent<1>();
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::z() const
{
  return GetComponent<2>();
}

EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::w() const
{
  return GetComponent<3>();
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::Get() const
{
  return _mm_shuffle_epi32(m_v, EZ_TO_SHUFFLE(s));
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator+(const ezSimdVec4u& v) const
{
  return _mm_add_epi32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator-(const ezSimdVec4u& v) const
{
  return _mm_sub_epi32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMul(const ezSimdVec4u& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_mullo_epi32(m_v, v.m_v);
#else
  EZ_ASSERT_NOT_IMPLEMENTED; // not sure whether this code works so better assert
  __m128i tmp1 = _mm_mul_epu32(m_v, v.m_v);
  __m128i tmp2 = _mm_mul_epu32(_mm_srli_si128(m_v, 4), _mm_srli_si128(v.m_v, 4));
  return _mm_unpacklo_epi32(_mm_shuffle_epi32(tmp1, EZ_SHUFFLE(0, 2, 0, 0)), _mm_shuffle_epi32(tmp2, EZ_SHUFFLE(0, 2, 0, 0)));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator|(const ezSimdVec4u& v) const
{
  return _mm_or_si128(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator&(const ezSimdVec4u& v) const
{
  return _mm_and_si128(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator^(const ezSimdVec4u& v) const
{
  return _mm_xor_si128(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator~() const
{
  __m128i ones = _mm_cmpeq_epi8(_mm_setzero_si128(), _mm_setzero_si128());
  return _mm_xor_si128(ones, m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator<<(ezUInt32 uiShift) const
{
  return _mm_slli_epi32(m_v, uiShift);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator>>(ezUInt32 uiShift) const
{
  return _mm_srli_epi32(m_v, uiShift);
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator+=(const ezSimdVec4u& v)
{
  m_v = _mm_add_epi32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator-=(const ezSimdVec4u& v)
{
  m_v = _mm_sub_epi32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator|=(const ezSimdVec4u& v)
{
  m_v = _mm_or_si128(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator&=(const ezSimdVec4u& v)
{
  m_v = _mm_and_si128(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator^=(const ezSimdVec4u& v)
{
  m_v = _mm_xor_si128(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator<<=(ezUInt32 uiShift)
{
  m_v = _mm_slli_epi32(m_v, uiShift);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator>>=(ezUInt32 uiShift)
{
  m_v = _mm_srli_epi32(m_v, uiShift);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMin(const ezSimdVec4u& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_min_epu32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmplt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMax(const ezSimdVec4u& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_max_epu32(m_v, v.m_v);
#else
  __m128i mask = _mm_cmpgt_epi32(m_v, v.m_v);
  return _mm_or_si128(_mm_and_si128(mask, m_v), _mm_andnot_si128(mask, v.m_v));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator==(const ezSimdVec4u& v) const
{
  return _mm_castsi128_ps(_mm_cmpeq_epi32(m_v, v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator!=(const ezSimdVec4u& v) const
{
  return !(*this == v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator<=(const ezSimdVec4u& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  __m128i minValue = _mm_min_epu32(m_v, v.m_v);
  return _mm_castsi128_ps(_mm_cmpeq_epi32(minValue, m_v));
#else
  return !(*this > v);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator<(const ezSimdVec4u& v) const
{
  __m128i signBit = _mm_set1_epi32(0x80000000);
  __m128i a = _mm_sub_epi32(m_v, signBit);
  __m128i b = _mm_sub_epi32(v.m_v, signBit);
  return _mm_castsi128_ps(_mm_cmplt_epi32(a, b));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator>=(const ezSimdVec4u& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  __m128i maxValue = _mm_max_epu32(m_v, v.m_v);
  return _mm_castsi128_ps(_mm_cmpeq_epi32(maxValue, m_v));
#else
  return !(*this < v);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator>(const ezSimdVec4u& v) const
{
  __m128i signBit = _mm_set1_epi32(0x80000000);
  __m128i a = _mm_sub_epi32(m_v, signBit);
  __m128i b = _mm_sub_epi32(v.m_v, signBit);
  return _mm_castsi128_ps(_mm_cmpgt_epi32(a, b));
}

// static
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::MakeZero()
{
  return _mm_setzero_si128();
}

// not needed atm
#if 0
void ezSimdVec4u::Transpose(ezSimdVec4u& v0, ezSimdVec4u& v1, ezSimdVec4u& v2, ezSimdVec4u& v3)
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
