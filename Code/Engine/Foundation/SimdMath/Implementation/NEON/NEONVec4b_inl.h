#pragma once

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool b)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_u32(b ? 0xFFFFFFFF : 0);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(bool x, bool y, bool z, bool w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) ezUInt32 mask[4] = {x ? 0xFFFFFFFF : 0, y ? 0xFFFFFFFF : 0, z ? 0xFFFFFFFF : 0, w ? 0xFFFFFFFF : 0};
  m_v = vld1q_u32(mask);
}

EZ_ALWAYS_INLINE ezSimdVec4b::ezSimdVec4b(ezInternal::QuadBool v)
{
  m_v = v;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::GetComponent() const
{
  return vgetq_lane_u32(m_v, N) & 1;
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
  return __builtin_shufflevector(m_v, m_v, EZ_TO_SHUFFLE(s));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator&&(const ezSimdVec4b& rhs) const
{
  return vandq_u32(m_v, rhs.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator||(const ezSimdVec4b& rhs) const
{
  return vorrq_u32(m_v, rhs.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator!() const
{
  return vmvnq_u32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator==(const ezSimdVec4b& rhs) const
{
  return vceqq_u32(m_v, rhs.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::operator!=(const ezSimdVec4b& rhs) const
{
  return veorq_u32(m_v, rhs.m_v);
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AllSet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMask(m_v) & mask) == mask;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::AnySet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMask(m_v) & mask) != 0;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4b::NoneSet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMask(m_v) & mask) == 0;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4b::Select(const ezSimdVec4b& vCmp, const ezSimdVec4b& vTrue, const ezSimdVec4b& vFalse)
{
  return vbslq_u32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}
