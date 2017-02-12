#pragma once

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b()
{
  EZ_CHECK_ALIGNMENT_16(this);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool b)
{
  EZ_CHECK_ALIGNMENT_16(this);

  ezUInt32 mask = b ? 0xFFFFFFFF : 0;
  __m128 tmp = _mm_load_ss((float*)&mask);
  m_v = _mm_shuffle_ps(tmp, tmp, _MM_SHUFFLE(0, 0, 0, 0));
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool x, bool y, bool z, bool w)
{
  EZ_CHECK_ALIGNMENT_16(this);

  ezUInt32 EZ_ALIGN_16(mask[4]) = { x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0 };
  m_v = _mm_load_ps((float*)mask);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(ezInternal::QuadBool v)
{
  m_v = v;
}

template<int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::GetComponent() const
{
  return _mm_movemask_ps(_mm_shuffle_ps(m_v, m_v, _MM_SHUFFLE(N, N, N, N))) != 0;
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::x() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::y() const
{
  return GetComponent<1>();
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::z() const
{
  return GetComponent<2>();
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::w() const
{
  return GetComponent<3>();
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::Get() const
{
  const int shuffle = ((s >> 12) & 0x03) | ((s >> 6) & 0x0c) | (s & 0x30) | ((s << 6) & 0xc0);
  return _mm_shuffle_ps(m_v, m_v, shuffle);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator&&(const ezSimdVec4b& rhs) const
{
  return _mm_and_ps(m_v, rhs.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator||(const ezSimdVec4b& rhs) const
{
  return _mm_or_ps(m_v, rhs.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator!() const
{
  __m128 allTrue = _mm_cmpeq_ps(_mm_setzero_ps(), _mm_setzero_ps());
  return _mm_xor_ps(m_v, allTrue);
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::AllSet()
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm_test_all_ones(_mm_castps_si128(m_v)) != 0;
#else
  return _mm_movemask_ps(m_v) == 0xF;
#endif
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::AnySet()
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm_testz_ps(m_v, m_v) == 0;
#else
  return _mm_movemask_ps(m_v) != 0;
#endif
}

EZ_ALWAYS_INLINE bool ezSimdVec4b::NoneSet()
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX
  return _mm_testz_ps(m_v, m_v) != 0;
#else
  return _mm_movemask_ps(m_v) == 0;
#endif
}

