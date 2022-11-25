#pragma once

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool b)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  ezUInt32 mask = b ? 0xFFFFFFFF : 0;
  __m128 tmp = _mm_load_ss((float*)&mask);
  m_v = _mm_shuffle_ps(tmp, tmp, EZ_TO_SHUFFLE(ezSwizzle::XXXX));
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool x, bool y, bool z, bool w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) ezUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = _mm_load_ps((float*)mask);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(ezInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::GetComponent() const
{
  return _mm_movemask_ps(_mm_shuffle_ps(m_v, m_v, EZ_SHUFFLE(N, N, N, N))) != 0;
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
  return _mm_shuffle_ps(m_v, m_v, EZ_TO_SHUFFLE(s));
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

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator==(const ezSimdVec4b& v) const
{
  return !(*this != v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator!=(const ezSimdVec4b& v) const
{
  return _mm_xor_ps(m_v, v.m_v);
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AllSet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == mask;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AnySet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) != 0;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::NoneSet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (_mm_movemask_ps(m_v) & mask) == 0;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::Select(const ezSimdVec4b& cmp, const ezSimdVec4b& ifTrue, const ezSimdVec4b& ifFalse)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_blendv_ps(ifFalse.m_v, ifTrue.m_v, cmp.m_v);
#else
  return _mm_or_ps(_mm_andnot_ps(cmp.m_v, ifFalse.m_v), _mm_and_ps(cmp.m_v, ifTrue.m_v));
#endif
}
