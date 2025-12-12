#pragma once

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool b)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
#if EZ_SSE_LEVEL >= EZ_SSE_AVX

  alignas(32) ezUInt64 mask[4] = {b ? 0xFFFFFFFFFFFFFFFFULL : 0, b ? 0xFFFFFFFFFFFFFFFFULL : 0, b ? 0xFFFFFFFFFFFFFFFFULL : 0, b ? 0xFFFFFFFFFFFFFFFFULL : 0};
  m_v = _mm256_load_pd((double*)mask);


#else

  __m128d val;
  if(b)
  {
    alignas(16) ezUInt64 trueVal[2] = {0xFFFFFFFFFFFFFFFFULL,0xFFFFFFFFFFFFFFFFULL};
    val = _mm_load_pd(((double*)(&trueVal)));
  }
  else
  {
    val = _mm_set1_pd(0.0);
  }

  m_v.xy = val;
  m_v.zw = val;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool x, bool y, bool z, bool w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  alignas(32) ezUInt64 mask[4] = {x ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, y ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, z ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, w ? 0xFFFFFFFFFFFFFFFFULL : 0ULL};
  m_v = _mm256_load_pd((double*)mask);
#else
  alignas(16) ezUInt64 mask[4] = {x ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, y ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, z ? 0xFFFFFFFFFFFFFFFFULL : 0ULL, w ? 0xFFFFFFFFFFFFFFFFULL : 0ULL};
  m_v.xy = _mm_load_pd((double*)&mask[0]);
  m_v.zw = _mm_load_pd((double*)&mask[2]);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(ezInternal::QuadBoolWide v)
{
  m_v = v;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::GetComponent() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return (_mm256_movemask_pd(m_v) & (1 << N)) != 0;
#else
  if constexpr (N < 2)
  {
    __m128d shuffled = _mm_shuffle_pd(m_v.xy, m_v.xy, N == 0 ? 0 : 3);
    return (_mm_movemask_pd(shuffled) & 1) != 0;
  }
  else
  {
    __m128d shuffled = _mm_shuffle_pd(m_v.zw, m_v.zw, N == 2 ? 0 : 3);
    return (_mm_movemask_pd(shuffled) & 1) != 0;
  }
#endif
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::x() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::y() const
{
  return GetComponent<1>();
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::z() const
{
  return GetComponent<2>();
}

EZ_ALWAYS_INLINE bool ezSimdVec4bWide::w() const
{
  return GetComponent<3>();
}


template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::Get() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  ezSimdVec4bWide result;
  EZ_WIDE_SHUFFLE_AVX1(m_v, m_v, EZ_TO_SHUFFLE(s), result.m_v);
  return result;
#else
  ezSimdVec4bWide result;
  EZ_WIDE_SHUFFLE_SSE(m_v.xy, m_v.zw, m_v.xy, m_v.zw, EZ_TO_SHUFFLE(s), result.m_v.xy, result.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator&&(const ezSimdVec4bWide& rhs) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_and_pd(m_v, rhs.m_v);
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_and_pd(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = _mm_and_pd(m_v.zw, rhs.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator||(const ezSimdVec4bWide& rhs) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_or_pd(m_v, rhs.m_v);
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_or_pd(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = _mm_or_pd(m_v.zw, rhs.m_v.zw);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  __m256d allTrue = _mm256_cmp_pd(_mm256_setzero_pd(), _mm256_setzero_pd(), _CMP_EQ_OQ);
  return _mm256_xor_pd(m_v, allTrue);
#else
  __m128d allTrue = _mm_cmpeq_pd(_mm_setzero_pd(), _mm_setzero_pd());
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_xor_pd(m_v.xy, allTrue);
  result.m_v.zw = _mm_xor_pd(m_v.zw, allTrue);
  return result;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator==(const ezSimdVec4bWide& rhs) const
{
  return !(*this != rhs);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!=(const ezSimdVec4bWide& rhs) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_xor_pd(m_v, rhs.m_v);
#else
  ezSimdVec4bWide result;
  result.m_v.xy = _mm_xor_pd(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = _mm_xor_pd(m_v.zw, rhs.m_v.zw);
  return result;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AllSet() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  const int mask = EZ_BIT(N) - 1;
  return (_mm256_movemask_pd(m_v) & mask) == mask;
#else
  int xy_bits = _mm_movemask_pd(m_v.xy);
  int zw_bits = _mm_movemask_pd(m_v.zw);
  int combined = (zw_bits << 2) | xy_bits;
  const int mask = EZ_BIT(N) - 1;
  return (combined & mask) == mask;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AnySet() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  const int mask = EZ_BIT(N) - 1;
  return (_mm256_movemask_pd(m_v) & mask) != 0;
#else
  int xy_bits = _mm_movemask_pd(m_v.xy);
  int zw_bits = _mm_movemask_pd(m_v.zw);
  int combined = (zw_bits << 2) | xy_bits;
  const int mask = EZ_BIT(N) - 1;
  return (combined & mask) != 0;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::NoneSet() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  const int mask = EZ_BIT(N) - 1;
  return (_mm256_movemask_pd(m_v) & mask) == 0;
#else
  int xy_bits = _mm_movemask_pd(m_v.xy);
  int zw_bits = _mm_movemask_pd(m_v.zw);
  int combined = (zw_bits << 2) | xy_bits;
  const int mask = EZ_BIT(N) - 1;
  return (combined & mask) == 0;
#endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::Select(const ezSimdVec4bWide& vCmp, const ezSimdVec4bWide& vTrue, const ezSimdVec4bWide& vFalse)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm256_blendv_pd(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  ezSimdVec4bWide result;
#if EZ_SSE_LEVEL >= EZ_SSE_41
  result.m_v.xy = _mm_blendv_pd(vFalse.m_v.xy, vTrue.m_v.xy, vCmp.m_v.xy);
  result.m_v.zw = _mm_blendv_pd(vFalse.m_v.zw, vTrue.m_v.zw, vCmp.m_v.zw);
#else
  result.m_v.xy = _mm_or_pd(_mm_andnot_pd(vCmp.m_v.xy, vFalse.m_v.xy), _mm_and_pd(vCmp.m_v.xy, vTrue.m_v.xy));
  result.m_v.zw = _mm_or_pd(_mm_andnot_pd(vCmp.m_v.zw, vFalse.m_v.zw), _mm_and_pd(vCmp.m_v.zw, vTrue.m_v.zw));
#endif
  return result;
#endif
}
