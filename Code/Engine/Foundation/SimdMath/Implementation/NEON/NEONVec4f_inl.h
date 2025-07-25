#pragma once

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = vmovq_n_f32(ezMath::NaN<float>());
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(float xyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_f32(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(const ezSimdFloat& xyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = xyzw.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(float x, float y, float z, float w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) float values[4] = {x, y, z, w};
  m_v = vld1q_f32(values);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float xyzw)
{
  m_v = vmovq_n_f32(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float x, float y, float z, float w)
{
  alignas(16) float values[4] = {x, y, z, w};
  m_v = vld1q_f32(values);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetX(const ezSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 0);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetY(const ezSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 1);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetZ(const ezSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 2);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetW(const ezSimdFloat& f)
{
  m_v = vsetq_lane_f32(f, m_v, 3);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetZero()
{
  m_v = vmovq_n_f32(0.0f);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<1>(const float* pFloat)
{
  m_v = vld1q_lane_f32(pFloat, vmovq_n_f32(0.0f), 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<2>(const float* pFloat)
{
  m_v = vreinterpretq_f32_f64(vld1q_lane_f64(reinterpret_cast<const float64_t*>(pFloat), vmovq_n_f64(0.0), 0));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<3>(const float* pFloat)
{
  m_v = vcombine_f32(vld1_f32(pFloat), vld1_lane_f32(pFloat + 2, vmov_n_f32(0.0f), 0));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<4>(const float* pFloat)
{
  m_v = vld1q_f32(pFloat);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<1>(float* pFloat) const
{
  vst1q_lane_f32(pFloat, m_v, 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<2>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<3>(float* pFloat) const
{
  vst1q_lane_f64(reinterpret_cast<float64_t*>(pFloat), vreinterpretq_f64_f32(m_v), 0);
  vst1q_lane_f32(pFloat + 2, m_v, 2);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<4>(float* pFloat) const
{
  vst1q_f32(pFloat, m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetReciprocal<ezMathAcc::BITS_12>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // One iteration of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);

  return x1;
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetReciprocal<ezMathAcc::BITS_23>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // Two iterations of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);
  float32x4_t x2 = vmulq_f32(vrecpsq_f32(m_v, x1), x1);

  return x2;
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetReciprocal<ezMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetInvSqrt<ezMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), vsqrtq_f32(m_v));
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetInvSqrt<ezMathAcc::BITS_23>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // Two iterations of Newton-Raphson
  const float32x4_t x1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, m_v), x1), x1);
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetInvSqrt<ezMathAcc::BITS_12>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // One iteration of Newton-Raphson
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetSqrt<ezMathAcc::BITS_12>() const
{
  return CompMul(GetInvSqrt<ezMathAcc::BITS_12>());
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetSqrt<ezMathAcc::BITS_23>() const
{
  return CompMul(GetInvSqrt<ezMathAcc::BITS_23>());
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetSqrt<ezMathAcc::FULL>() const
{
  return vsqrtq_f32(m_v);
}

template <int N, ezMathAcc::Enum acc>
void ezSimdVec4f::NormalizeIfNotZero(const ezSimdFloat& fEpsilon)
{
  ezSimdFloat sqLength = GetLengthSquared<N>();
  uint32x4_t isNotZero = vcgtq_f32(sqLength.m_v, fEpsilon.m_v);
  m_v = vmulq_f32(m_v, sqLength.GetInvSqrt<acc>().m_v);
  m_v = vreinterpretq_f32_u32(vandq_u32(isNotZero, vreinterpretq_u32_f32(m_v)));
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsZero() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMask(vceqzq_f32(m_v)) & mask) == mask;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsZero(const ezSimdFloat& fEpsilon) const
{
  const int mask = EZ_BIT(N) - 1;
  float32x4_t absVal = Abs().m_v;
  return (ezInternal::NeonMoveMask(vcltq_f32(absVal, fEpsilon.m_v)) & mask) == mask;
}

template <int N>
inline bool ezSimdVec4f::IsNaN() const
{
  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMask(vceqq_f32(m_v, m_v)) & mask) != mask;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsValid() const
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  uint32x4_t exponentMask = vmovq_n_u32(0x7f800000);

  uint32x4_t exponentIs1 = vceqq_u32(vandq_u32(vreinterpretq_u32_f32(m_v), exponentMask), exponentMask);

  const int mask = EZ_BIT(N) - 1;
  return (ezInternal::NeonMoveMask(exponentIs1) & mask) == 0;
}

template <int N>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetComponent() const
{
  return vdupq_laneq_f32(m_v, N);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::x() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::y() const
{
  return GetComponent<1>();
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::z() const
{
  return GetComponent<2>();
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::w() const
{
  return GetComponent<3>();
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Get() const
{
  return __builtin_shufflevector(m_v, m_v, EZ_TO_SHUFFLE(s));
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetCombined(const ezSimdVec4f& other) const
{
  return __builtin_shufflevector(m_v, other.m_v, EZ_TO_SHUFFLE(s));
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator-() const
{
  return vnegq_f32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator+(const ezSimdVec4f& v) const
{
  return vaddq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator-(const ezSimdVec4f& v) const
{
  return vsubq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator*(const ezSimdFloat& f) const
{
  return vmulq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator/(const ezSimdFloat& f) const
{
  return vdivq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMul(const ezSimdVec4f& v) const
{
  return vmulq_f32(m_v, v.m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompDiv<ezMathAcc::FULL>(const ezSimdVec4f& v) const
{
  return vdivq_f32(m_v, v.m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompDiv<ezMathAcc::BITS_23>(const ezSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<ezMathAcc::BITS_23>());
}

template <>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompDiv<ezMathAcc::BITS_12>(const ezSimdVec4f& v) const
{
  return CompMul(v.GetReciprocal<ezMathAcc::BITS_12>());
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMin(const ezSimdVec4f& v) const
{
  return vminq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMax(const ezSimdVec4f& v) const
{
  return vmaxq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Abs() const
{
  return vabsq_f32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Round() const
{
  return vrndnq_f32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Floor() const
{
  return vrndmq_f32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Ceil() const
{
  return vrndpq_f32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Trunc() const
{
  return vrndq_f32(m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::FlipSign(const ezSimdVec4b& cmp) const
{
  return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(m_v), vshlq_n_u32(cmp.m_v, 31)));
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Select(const ezSimdVec4b& cmp, const ezSimdVec4f& ifTrue, const ezSimdVec4f& ifFalse)
{
  return vbslq_f32(cmp.m_v, ifTrue.m_v, ifFalse.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator+=(const ezSimdVec4f& v)
{
  m_v = vaddq_f32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator-=(const ezSimdVec4f& v)
{
  m_v = vsubq_f32(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator*=(const ezSimdFloat& f)
{
  m_v = vmulq_f32(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator/=(const ezSimdFloat& f)
{
  m_v = vdivq_f32(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator==(const ezSimdVec4f& v) const
{
  return vceqq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator!=(const ezSimdVec4f& v) const
{
  return vmvnq_u32(vceqq_f32(m_v, v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator<=(const ezSimdVec4f& v) const
{
  return vcleq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator<(const ezSimdVec4f& v) const
{
  return vcltq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator>=(const ezSimdVec4f& v) const
{
  return vcgeq_f32(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator>(const ezSimdVec4f& v) const
{
  return vcgtq_f32(m_v, v.m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<2>() const
{
  return vpadds_f32(vget_low_f32(m_v));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<4>() const
{
  float32x2_t x0 = vpadd_f32(vget_low_f32(m_v), vget_high_f32(m_v));
  return vpadds_f32(x0);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<2>() const
{
  return vpmins_f32(vget_low_f32(m_v));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<3>() const
{
  return vminq_f32(vmovq_n_f32(vpmins_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<4>() const
{
  return vpmins_f32(vpmin_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<2>() const
{
  return vpmaxs_f32(vget_low_f32(m_v));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<3>() const
{
  return vmaxq_f32(vmovq_n_f32(vpmaxs_f32(vget_low_f32(m_v))), vdupq_laneq_f32(m_v, 2));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<4>() const
{
  return vpmaxs_f32(vpmax_f32(vget_low_f32(m_v), vget_high_f32(m_v)));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot<1>(const ezSimdVec4f& v) const
{
  return vdupq_laneq_f32(vmulq_f32(m_v, v.m_v), 0);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot<2>(const ezSimdVec4f& v) const
{
  return vpadds_f32(vmul_f32(vget_low_f32(m_v), vget_low_f32(v.m_v)));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot<3>(const ezSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<3>();
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot<4>(const ezSimdVec4f& v) const
{
  return CompMul(v).HorizontalSum<4>();
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CrossRH(const ezSimdVec4f& v) const
{
  float32x4_t a = vmulq_f32(m_v, __builtin_shufflevector(v.m_v, v.m_v, EZ_TO_SHUFFLE(ezSwizzle::YZXW)));
  float32x4_t b = vmulq_f32(v.m_v, __builtin_shufflevector(m_v, m_v, EZ_TO_SHUFFLE(ezSwizzle::YZXW)));
  float32x4_t c = vsubq_f32(a, b);

  return __builtin_shufflevector(c, c, EZ_TO_SHUFFLE(ezSwizzle::YZXW));
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulAdd(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulAdd(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c)
{
  return vfmaq_f32(c.m_v, a.m_v, b.m_v);
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulSub(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulSub(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c)
{
  return vnegq_f32(vfmsq_f32(c.m_v, a.m_v, b.m_v));
}

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CopySign(const ezSimdVec4f& magnitude, const ezSimdVec4f& sign)
{
  return vbslq_f32(vmovq_n_u32(0x80000000), sign.m_v, magnitude.m_v);
}
