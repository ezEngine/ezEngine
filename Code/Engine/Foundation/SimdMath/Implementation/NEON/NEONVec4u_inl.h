#pragma once

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezUInt32 xyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_u32(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) ezUInt32 values[4] = {x, y, z, w};
  m_v = vld1q_u32(values);
}

EZ_ALWAYS_INLINE ezSimdVec4u::ezSimdVec4u(ezInternal::QuadUInt v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE void ezSimdVec4u::Set(ezUInt32 xyzw)
{
  m_v = vmovq_n_u32(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4u::Set(ezUInt32 x, ezUInt32 y, ezUInt32 z, ezUInt32 w)
{
  alignas(16) ezUInt32 values[4] = {x, y, z, w};
  m_v = vld1q_u32(values);
}

EZ_ALWAYS_INLINE void ezSimdVec4u::SetZero()
{
  m_v = vmovq_n_u32(0);
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
  return vcvtq_f32_u32(m_v);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::Truncate(const ezSimdVec4f& f)
{
  return vcvtq_u32_f32(f.m_v);
}

template <int N>
EZ_ALWAYS_INLINE ezUInt32 ezSimdVec4u::GetComponent() const
{
  return vgetq_lane_u32(m_v, N);
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
  return __builtin_shufflevector(m_v, m_v, EZ_TO_SHUFFLE(s));
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator+(const ezSimdVec4u& v) const
{
  return vaddq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator-(const ezSimdVec4u& v) const
{
  return vsubq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMul(const ezSimdVec4u& v) const
{
  return vmulq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator|(const ezSimdVec4u& v) const
{
  return vorrq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator&(const ezSimdVec4u& v) const
{
  return vandq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator^(const ezSimdVec4u& v) const
{
  return veorq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator~() const
{
  return vmvnq_u32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator<<(ezUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(uiShift));
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::operator>>(ezUInt32 uiShift) const
{
  return vshlq_u32(m_v, vmovq_n_u32(-uiShift));
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator+=(const ezSimdVec4u& v)
{
  m_v = vaddq_u32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator-=(const ezSimdVec4u& v)
{
  m_v = vsubq_u32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator|=(const ezSimdVec4u& v)
{
  m_v = vorrq_u32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator&=(const ezSimdVec4u& v)
{
  m_v = vandq_u32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator^=(const ezSimdVec4u& v)
{
  m_v = veorq_u32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator<<=(ezUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(uiShift));
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u& ezSimdVec4u::operator>>=(ezUInt32 uiShift)
{
  m_v = vshlq_u32(m_v, vmovq_n_u32(-uiShift));
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMin(const ezSimdVec4u& v) const
{
  return vminq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::CompMax(const ezSimdVec4u& v) const
{
  return vmaxq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator==(const ezSimdVec4u& v) const
{
  return vceqq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator!=(const ezSimdVec4u& v) const
{
  return vmvnq_u32(vceqq_u32(m_v, v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator<=(const ezSimdVec4u& v) const
{
  return vcleq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator<(const ezSimdVec4u& v) const
{
  return vcltq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator>=(const ezSimdVec4u& v) const
{
  return vcgeq_u32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4u::operator>(const ezSimdVec4u& v) const
{
  return vcgtq_u32(m_v, v.m_v);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4u ezSimdVec4u::MakeZero()
{
  return vmovq_n_u32(0);
}

// not needed atm
#if 0
void ezSimdVec4u::Transpose(ezSimdVec4u& v0, ezSimdVec4u& v1, ezSimdVec4u& v2, ezSimdVec4u& v3)
{
  uint32x4x2_t P0 = vzipq_u32(v0.m_v, v2.m_v);
  uint32x4x2_t P1 = vzipq_u32(v1.m_v, v3.m_v);

  uint32x4x2_t T0 = vzipq_u32(P0.val[0], P1.val[0]);
  uint32x4x2_t T1 = vzipq_u32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif
