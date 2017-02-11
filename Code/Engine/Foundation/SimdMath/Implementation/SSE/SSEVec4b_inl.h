#pragma once

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b()
{
  EZ_CHECK_ALIGNMENT_16(this);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool b)
{
  EZ_CHECK_ALIGNMENT_16(this);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(ezInternal::QuadBool v)
{
  m_v = v;
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
  ezSimdVec4b allTrue(true);
  return _mm_xor_ps(m_v, allTrue.m_v);
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

