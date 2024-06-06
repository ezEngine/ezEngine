#pragma once

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  m_v = vmovq_n_u32(0xCDCDCDCD);
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 xyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_s32(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) ezInt32 values[4] = {x, y, z, w};
  m_v = vld1q_s32(values);
}

EZ_ALWAYS_INLINE ezSimdVec4i::ezSimdVec4i(ezInternal::QuadInt v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::MakeZero()
{
  return vmovq_n_s32(0);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 xyzw)
{
  m_v = vmovq_n_s32(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::Set(ezInt32 x, ezInt32 y, ezInt32 z, ezInt32 w)
{
  alignas(16) ezInt32 values[4] = {x, y, z, w};
  m_v = vld1q_s32(values);
}

EZ_ALWAYS_INLINE void ezSimdVec4i::SetZero()
{
  m_v = vmovq_n_s32(0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<1>(const ezInt32* pInts)
{
  m_v = vld1q_lane_s32(pInts, vmovq_n_s32(0), 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<2>(const ezInt32* pInts)
{
  m_v = vreinterpretq_s32_s64(vld1q_lane_s64(reinterpret_cast<const int64_t*>(pInts), vmovq_n_s64(0), 0));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<3>(const ezInt32* pInts)
{
  m_v = vcombine_s32(vld1_s32(pInts), vld1_lane_s32(pInts + 2, vmov_n_s32(0), 0));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Load<4>(const ezInt32* pInts)
{
  m_v = vld1q_s32(pInts);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<1>(ezInt32* pInts) const
{
  vst1q_lane_s32(pInts, m_v, 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<2>(ezInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<3>(ezInt32* pInts) const
{
  vst1q_lane_s64(reinterpret_cast<int64_t*>(pInts), vreinterpretq_s64_s32(m_v), 0);
  vst1q_lane_s32(pInts + 2, m_v, 2);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4i::Store<4>(ezInt32* pInts) const
{
  vst1q_s32(pInts, m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4i::ToFloat() const
{
  return vcvtq_f32_s32(m_v);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Truncate(const ezSimdVec4f& f)
{
  return vcvtq_s32_f32(f.m_v);
}

template <int N>
EZ_ALWAYS_INLINE ezInt32 ezSimdVec4i::GetComponent() const
{
  return vgetq_lane_s32(m_v, N);
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
  return __builtin_shufflevector(m_v, m_v, EZ_TO_SHUFFLE(s));
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::GetCombined(const ezSimdVec4i& other) const
{
  return __builtin_shufflevector(m_v, other.m_v, EZ_TO_SHUFFLE(s));
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator-() const
{
  return vnegq_s32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator+(const ezSimdVec4i& v) const
{
  return vaddq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator-(const ezSimdVec4i& v) const
{
  return vsubq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMul(const ezSimdVec4i& v) const
{
  return vmulq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompDiv(const ezSimdVec4i& v) const
{
  int a[4];
  int b[4];
  Store<4>(a);
  v.Store<4>(b);

  for (ezUInt32 i = 0; i < 4; ++i)
  {
    a[i] = a[i] / b[i];
  }

  ezSimdVec4i r;
  r.Load<4>(a);
  return r;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator|(const ezSimdVec4i& v) const
{
  return vorrq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator&(const ezSimdVec4i& v) const
{
  return vandq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator^(const ezSimdVec4i& v) const
{
  return veorq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator~() const
{
  return vmvnq_s32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator<<(ezUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(uiShift));
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator>>(ezUInt32 uiShift) const
{
  return vshlq_s32(m_v, vmovq_n_s32(-uiShift));
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator<<(const ezSimdVec4i& v) const
{
  return vshlq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::operator>>(const ezSimdVec4i& v) const
{
  return vshlq_s32(m_v, vnegq_s32(v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator+=(const ezSimdVec4i& v)
{
  m_v = vaddq_s32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator-=(const ezSimdVec4i& v)
{
  m_v = vsubq_s32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator|=(const ezSimdVec4i& v)
{
  m_v = vorrq_s32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator&=(const ezSimdVec4i& v)
{
  m_v = vandq_s32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator^=(const ezSimdVec4i& v)
{
  m_v = veorq_s32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator<<=(ezUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(uiShift));
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i& ezSimdVec4i::operator>>=(ezUInt32 uiShift)
{
  m_v = vshlq_s32(m_v, vmovq_n_s32(-uiShift));
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMin(const ezSimdVec4i& v) const
{
  return vminq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::CompMax(const ezSimdVec4i& v) const
{
  return vmaxq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Abs() const
{
  return vabsq_s32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator==(const ezSimdVec4i& v) const
{
  return vceqq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator!=(const ezSimdVec4i& v) const
{
  return vmvnq_u32(vceqq_s32(m_v, v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator<=(const ezSimdVec4i& v) const
{
  return vcleq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator<(const ezSimdVec4i& v) const
{
  return vcltq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator>=(const ezSimdVec4i& v) const
{
  return vcgeq_s32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4i::operator>(const ezSimdVec4i& v) const
{
  return vcgtq_s32(m_v, v.m_v);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4i ezSimdVec4i::Select(const ezSimdVec4b& vCmp, const ezSimdVec4i& vTrue, const ezSimdVec4i& vFalse)
{
  return vbslq_s32(vCmp.m_v, vTrue.m_v, vFalse.m_v);
}

// not needed atm
#if 0
void ezSimdVec4i::Transpose(ezSimdVec4i& v0, ezSimdVec4i& v1, ezSimdVec4i& v2, ezSimdVec4i& v3)
{
  int32x4x2_t P0 = vzipq_s32(v0.m_v, v2.m_v);
  int32x4x2_t P1 = vzipq_s32(v1.m_v, v3.m_v);

  int32x4x2_t T0 = vzipq_s32(P0.val[0], P1.val[0]);
  int32x4x2_t T1 = vzipq_s32(P0.val[1], P1.val[1]);

  v0.m_v = T0.val[0];
  v1.m_v = T0.val[1];
  v2.m_v = T1.val[0];
  v3.m_v = T1.val[1];
}
#endif
