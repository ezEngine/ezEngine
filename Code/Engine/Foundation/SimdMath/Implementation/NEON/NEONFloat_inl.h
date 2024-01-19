#pragma once

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = vmovq_n_f32(ezMath::NaN<float>());
#endif
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(float f)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_f32(f);
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vcvtq_f32_s32(vmovq_n_s32(i));
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezUInt32 i)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vcvtq_f32_u32(vmovq_n_u32(i));
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezAngle a)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_f32(a.GetRadian());
}

EZ_ALWAYS_INLINE ezSimdFloat::ezSimdFloat(ezInternal::QuadFloat v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdFloat::operator float() const
{
  return vgetq_lane_f32(m_v, 0);
}

// static
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::MakeZero()
{
  return vmovq_n_f32(0.0f);
}

// static
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::MakeNaN()
{
  return vmovq_n_f32(ezMath::NaN<float>());
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator+(const ezSimdFloat& f) const
{
  return vaddq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator-(const ezSimdFloat& f) const
{
  return vsubq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator*(const ezSimdFloat& f) const
{
  return vmulq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::operator/(const ezSimdFloat& f) const
{
  return vdivq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator+=(const ezSimdFloat& f)
{
  m_v = vaddq_f32(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator-=(const ezSimdFloat& f)
{
  m_v = vsubq_f32(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator*=(const ezSimdFloat& f)
{
  m_v = vmulq_f32(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdFloat& ezSimdFloat::operator/=(const ezSimdFloat& f)
{
  m_v = vdivq_f32(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::IsEqual(const ezSimdFloat& rhs, const ezSimdFloat& fEpsilon) const
{
  ezSimdFloat minusEps = rhs - fEpsilon;
  ezSimdFloat plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator==(const ezSimdFloat& f) const
{
  return vgetq_lane_u32(vceqq_f32(m_v, f.m_v), 0) & 1;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator!=(const ezSimdFloat& f) const
{
  return !operator==(f);
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>=(const ezSimdFloat& f) const
{
  return vgetq_lane_u32(vcgeq_f32(m_v, f.m_v), 0) & 1;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>(const ezSimdFloat& f) const
{
  return vgetq_lane_u32(vcgtq_f32(m_v, f.m_v), 0) & 1;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<=(const ezSimdFloat& f) const
{
  return vgetq_lane_u32(vcleq_f32(m_v, f.m_v), 0) & 1;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<(const ezSimdFloat& f) const
{
  return vgetq_lane_u32(vcltq_f32(m_v, f.m_v), 0) & 1;
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator==(float f) const
{
  return (*this) == ezSimdFloat(f);
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator!=(float f) const
{
  return (*this) != ezSimdFloat(f);
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>(float f) const
{
  return (*this) > ezSimdFloat(f);
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator>=(float f) const
{
  return (*this) >= ezSimdFloat(f);
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<(float f) const
{
  return (*this) < ezSimdFloat(f);
}

EZ_ALWAYS_INLINE bool ezSimdFloat::operator<=(float f) const
{
  return (*this) <= ezSimdFloat(f);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetReciprocal<ezMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetReciprocal<ezMathAcc::BITS_23>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // Two iterations of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);
  float32x4_t x2 = vmulq_f32(vrecpsq_f32(m_v, x1), x1);

  return x2;
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetReciprocal<ezMathAcc::BITS_12>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // One iteration of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);

  return x1;
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetInvSqrt<ezMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), vsqrtq_f32(m_v));
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetInvSqrt<ezMathAcc::BITS_23>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // Two iterations of Newton-Raphson
  const float32x4_t x1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, m_v), x1), x1);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetInvSqrt<ezMathAcc::BITS_12>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // One iteration of Newton-Raphson
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetSqrt<ezMathAcc::FULL>() const
{
  return vsqrtq_f32(m_v);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetSqrt<ezMathAcc::BITS_23>() const
{
  return (*this) * GetInvSqrt<ezMathAcc::BITS_23>();
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::GetSqrt<ezMathAcc::BITS_12>() const
{
  return (*this) * GetInvSqrt<ezMathAcc::BITS_12>();
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::Max(const ezSimdFloat& f) const
{
  return vmaxq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::Min(const ezSimdFloat& f) const
{
  return vminq_f32(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdFloat::Abs() const
{
  return vabsq_f32(m_v);
}
