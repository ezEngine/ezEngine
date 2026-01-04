#pragma once

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool b)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vmovq_n_u64(b ? 0xFFFFFFFFFFFFFFFFULL : 0);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(bool x, bool y, bool z, bool w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) ezUInt64 mask[4] = {x ? 0xFFFFFFFFFFFFFFFFULL : 0, y ? 0xFFFFFFFFFFFFFFFFULL : 0, z ? 0xFFFFFFFFFFFFFFFFULL : 0, w ? 0xFFFFFFFFFFFFFFFFULL : 0};
  m_v.xy = vld1q_u64(reinterpret_cast<unsigned long *>(mask));
  m_v.zw = vld1q_u64(reinterpret_cast<unsigned long *>(mask + 2));
}

EZ_ALWAYS_INLINE ezSimdVec4bWide::ezSimdVec4bWide(ezInternal::QuadBoolWide v)
{
  m_v = v;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::GetComponent() const
{
  if constexpr (N < 2)
  {
    return vgetq_lane_u64(m_v.xy, N) != 0;
  }
  else
  {
    return vgetq_lane_u64(m_v.zw, N - 2) != 0;
  }
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
  ezSimdVec4bWide result;

  // For simplicity, implement basic shuffle
  // This might need adjustment for full swizzle support
  const uint64_t* src = (const uint64_t*)&m_v;
  uint64_t* dst = (uint64_t*)&result.m_v;

  dst[0] = src[(s >> 12) & 3];
  dst[1] = src[(s >> 8) & 3];
  dst[2] = src[(s >> 4) & 3];
  dst[3] = src[s & 3];

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator&&(const ezSimdVec4bWide& rhs) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = vandq_u64(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = vandq_u64(m_v.zw, rhs.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator||(const ezSimdVec4bWide& rhs) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = vorrq_u64(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = vorrq_u64(m_v.zw, rhs.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!() const
{
  ezSimdVec4bWide result;
  result.m_v.xy = veorq_u64(m_v.xy, vmovq_n_u64(~0ULL));
  result.m_v.zw = veorq_u64(m_v.zw, vmovq_n_u64(~0ULL));
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator==(const ezSimdVec4bWide& rhs) const
{
  return !(*this != rhs);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::operator!=(const ezSimdVec4bWide& rhs) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = veorq_u64(m_v.xy, rhs.m_v.xy);
  result.m_v.zw = veorq_u64(m_v.zw, rhs.m_v.zw);
  return result;
}

namespace ezInternal
{
  EZ_ALWAYS_INLINE uint32_t NeonMoveMaskWide(const QuadBoolWide& v)
  {
    uint32_t mask = 0;
    mask |= (vgetq_lane_u64(v.xy, 0) != 0) ? 1 : 0;
    mask |= (vgetq_lane_u64(v.xy, 1) != 0) ? 2 : 0;
    mask |= (vgetq_lane_u64(v.zw, 0) != 0) ? 4 : 0;
    mask |= (vgetq_lane_u64(v.zw, 1) != 0) ? 8 : 0;
    return mask;
  }
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AllSet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMaskWide(m_v) & mask) == mask;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::AnySet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMaskWide(m_v) & mask) != 0;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4bWide::NoneSet() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMaskWide(m_v) & mask) == 0;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4bWide::Select(const ezSimdVec4bWide& vCmp, const ezSimdVec4bWide& vTrue, const ezSimdVec4bWide& vFalse)
{
  ezSimdVec4bWide result;
  result.m_v.xy = vbslq_u64(vCmp.m_v.xy, vTrue.m_v.xy, vFalse.m_v.xy);
  result.m_v.zw = vbslq_u64(vCmp.m_v.zw, vTrue.m_v.zw, vFalse.m_v.zw);
  return result;
}
