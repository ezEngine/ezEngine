#pragma once

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool b)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  ezUInt32 mask = b ? 0xFFFFFFFF : 0;
  __m128 tmp = _mm_load_ss((float*)&mask);
  m_v = _mm_shuffle_ps(tmp, tmp, EZ_TO_SHUFFLE(ezSwizzle::XXXX));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool x, bool y, bool z, bool w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  alignas(16) ezUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = _mm_load_ps((float*)mask);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(ezInternal::QuadBool v)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  m_v = v;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::GetComponent() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  return _mm_movemask_ps(_mm_shuffle_ps(m_v, m_v, EZ_SHUFFLE(N, N, N, N))) != 0;
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
#error
#else
  return _mm_shuffle_ps(m_v, m_v, EZ_TO_SHUFFLE(s));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator&&(const ezSimdVec4bWide& rhs) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  return _mm_and_ps(m_v, rhs.m_v);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator||(const ezSimdVec4bWide& rhs) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  return _mm_or_ps(m_v, rhs.m_v);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  __m128 allTrue = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_setzero_ps());
  return _mm_xor_ps(m_v, allTrue);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator==(const ezSimdVec4bWide& rhs) const
{
  return !(*this != rhs);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!=(const ezSimdVec4bWide& rhs) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  return _mm_xor_ps(m_v, rhs.m_v);
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AllSet() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  const int mask = EZ_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == mask;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AnySet() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  const int mask = EZ_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) != 0;
#endif
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::NoneSet() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else
  const int mask = EZ_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == 0;
  #endif
}

// static
EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::Select(const ezSimdVec4bWide& vCmp, const ezSimdVec4bWide& vTrue, const ezSimdVec4bWide& vFalse)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
#error
#else

#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_blendv_ps(vFalse.m_v, vTrue.m_v, vCmp.m_v);
#else
  return _mm_or_ps(_mm_andnot_ps(vCmp.m_v, vFalse.m_v), _mm_and_ps(vCmp.m_v, vTrue.m_v));
#endif

#endif
}
