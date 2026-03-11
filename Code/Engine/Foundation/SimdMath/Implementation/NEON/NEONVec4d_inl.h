#pragma once

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d()
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

#if EZ_ENABLED(EZ_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v.xy = vdupq_n_f64(ezMath::NaN<double>());
  m_v.zw = m_v.xy;
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(fXyzw);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v.xy = vdupq_n_f64(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(const ezSimdDouble& fXyzw)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  m_v = fXyzw.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(float x, float y, float z, float w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) double vals[4] = {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w)};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(double x, double y, double z, double w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) double vals[4] = {x, y, z, w};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(int x, int y, int z, int w)
{
  EZ_CHECK_SIMD_ALIGNMENT(this);

  alignas(16) double vals[4] = {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w)};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double fXyzw)
{
  m_v.xy = vdupq_n_f64(fXyzw);
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float fXyzw)
{
  m_v.xy = vdupq_n_f64(static_cast<double>(fXyzw));
  m_v.zw = m_v.xy;
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(float x, float y, float z, float w)
{
  alignas(16) double vals[4] = {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w)};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(double x, double y, double z, double w)
{
  alignas(16) double vals[4] = {x, y, z, w};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::Set(int x, int y, int z, int w)
{
  alignas(16) double vals[4] = {static_cast<double>(x), static_cast<double>(y), static_cast<double>(z), static_cast<double>(w)};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetX(const ezSimdDouble& f)
{
  m_v.xy = vcopyq_laneq_f64(m_v.xy, 0, f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetY(const ezSimdDouble& f)
{
  m_v.xy = vcopyq_laneq_f64(m_v.xy, 1, f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZ(const ezSimdDouble& f)
{
  m_v.zw = vcopyq_laneq_f64(m_v.zw, 0, f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetW(const ezSimdDouble& f)
{
  m_v.zw = vcopyq_laneq_f64(m_v.zw, 1, f.m_v.xy, 0);
}

EZ_ALWAYS_INLINE void ezSimdVec4d::SetZero()
{
  m_v.xy = vdupq_n_f64(0.0);
  m_v.zw = m_v.xy;
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<1>(const double* pDoubles)
{
  m_v.xy = vld1q_lane_f64(pDoubles, vdupq_n_f64(0.0), 0);
  m_v.zw = vdupq_n_f64(0.0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<2>(const double* pDoubles)
{
  m_v.xy = vld1q_f64(pDoubles);
  m_v.zw = vdupq_n_f64(0.0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<3>(const double* pDoubles)
{
  m_v.xy = vld1q_f64(pDoubles);
  m_v.zw = vld1q_lane_f64(pDoubles + 2, vdupq_n_f64(0.0), 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<4>(const double* pDoubles)
{
  m_v.xy = vld1q_f64(pDoubles);
  m_v.zw = vld1q_f64(pDoubles + 2);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<1>(const float* pFloats)
{
  m_v.xy = vsetq_lane_f64(static_cast<double>(pFloats[0]), vdupq_n_f64(0.0), 0);
  m_v.zw = vdupq_n_f64(0.0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<2>(const float* pFloats)
{
  alignas(16) double vals[2] = {static_cast<double>(pFloats[0]), static_cast<double>(pFloats[1])};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vdupq_n_f64(0.0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<3>(const float* pFloats)
{
  alignas(16) double vals[4] = {static_cast<double>(pFloats[0]), static_cast<double>(pFloats[1]), static_cast<double>(pFloats[2]), 0.0};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Load<4>(const float* pFloats)
{
  alignas(16) double vals[4] = {static_cast<double>(pFloats[0]), static_cast<double>(pFloats[1]), static_cast<double>(pFloats[2]), static_cast<double>(pFloats[3])};
  m_v.xy = vld1q_f64(vals);
  m_v.zw = vld1q_f64(vals + 2);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<1>(double* pDoubles) const
{
  vst1q_lane_f64(pDoubles, m_v.xy, 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<2>(double* pDoubles) const
{
  vst1q_f64(pDoubles, m_v.xy);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<3>(double* pDoubles) const
{
  vst1q_f64(pDoubles, m_v.xy);
  vst1q_lane_f64(pDoubles + 2, m_v.zw, 0);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<4>(double* pDoubles) const
{
  vst1q_f64(pDoubles, m_v.xy);
  vst1q_f64(pDoubles + 2, m_v.zw);
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<1>(float* pFloats) const
{
  pFloats[0] = static_cast<float>(vgetq_lane_f64(m_v.xy, 0));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<2>(float* pFloats) const
{
  pFloats[0] = static_cast<float>(vgetq_lane_f64(m_v.xy, 0));
  pFloats[1] = static_cast<float>(vgetq_lane_f64(m_v.xy, 1));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<3>(float* pFloats) const
{
  pFloats[0] = static_cast<float>(vgetq_lane_f64(m_v.xy, 0));
  pFloats[1] = static_cast<float>(vgetq_lane_f64(m_v.xy, 1));
  pFloats[2] = static_cast<float>(vgetq_lane_f64(m_v.zw, 0));
}

template <>
EZ_ALWAYS_INLINE void ezSimdVec4d::Store<4>(float* pFloats) const
{
  pFloats[0] = static_cast<float>(vgetq_lane_f64(m_v.xy, 0));
  pFloats[1] = static_cast<float>(vgetq_lane_f64(m_v.xy, 1));
  pFloats[2] = static_cast<float>(vgetq_lane_f64(m_v.zw, 0));
  pFloats[3] = static_cast<float>(vgetq_lane_f64(m_v.zw, 1));
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetReciprocal() const
{
  ezSimdVec4d result;
  float64x2_t one = vdupq_n_f64(1.0);
  result.m_v.xy = vdivq_f64(one, m_v.xy);
  result.m_v.zw = vdivq_f64(one, m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetSqrt() const
{
  ezSimdVec4d result;
  result.m_v.xy = vsqrtq_f64(m_v.xy);
  result.m_v.zw = vsqrtq_f64(m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetInvSqrt() const
{
  ezSimdVec4d result;
  float64x2_t one = vdupq_n_f64(1.0);
  result.m_v.xy = vdivq_f64(one, vsqrtq_f64(m_v.xy));
  result.m_v.zw = vdivq_f64(one, vsqrtq_f64(m_v.zw));
  return result;
}

template <int N>
void ezSimdVec4d::NormalizeIfNotZero(const ezSimdDouble& fEpsilon)
{
  ezSimdDouble sqLength = GetLengthSquared<N>();
  ezSimdVec4bWide isNotZero;
  isNotZero.m_v.xy = vcgtq_f64(sqLength.m_v.xy, fEpsilon.m_v.xy);
  isNotZero.m_v.zw = isNotZero.m_v.xy;
  ezSimdDouble invSqrt = sqLength.GetInvSqrt();
  *this = Select(isNotZero, *this * invSqrt, MakeZero());
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsZero() const
{
  const int mask = EZ_BIT(N) - 1;
  float64x2_t zero = vdupq_n_f64(0.0);
  uint64x2_t cmpXY = vceqq_f64(m_v.xy, zero);
  uint64x2_t cmpZW = vceqq_f64(m_v.zw, zero);
  int m1 = (vgetq_lane_u64(cmpXY, 0) ? 1 : 0) | (vgetq_lane_u64(cmpXY, 1) ? 2 : 0);
  int m2 = (vgetq_lane_u64(cmpZW, 0) ? 1 : 0) | (vgetq_lane_u64(cmpZW, 1) ? 2 : 0);
  return ((m1 | (m2 << 2)) & mask) == mask;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsZero(const ezSimdDouble& fEpsilon) const
{
  const int mask = EZ_BIT(N) - 1;
  float64x2_t eps = vdupq_n_f64(static_cast<double>(fEpsilon));
  float64x2_t absXY = vabsq_f64(m_v.xy);
  float64x2_t absZW = vabsq_f64(m_v.zw);
  uint64x2_t cmpXY = vcltq_f64(absXY, eps);
  uint64x2_t cmpZW = vcltq_f64(absZW, eps);
  int m1 = (vgetq_lane_u64(cmpXY, 0) ? 1 : 0) | (vgetq_lane_u64(cmpXY, 1) ? 2 : 0);
  int m2 = (vgetq_lane_u64(cmpZW, 0) ? 1 : 0) | (vgetq_lane_u64(cmpZW, 1) ? 2 : 0);
  return ((m1 | (m2 << 2)) & mask) == mask;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsNaN() const
{
  // IEEE 754: NaN != NaN, so a == a is false if a is NaN
  // vceqq_f64 returns 0 for NaN lanes, XOR inverts to detect them
  const int mask = EZ_BIT(N) - 1;
  uint64x2_t allOnes = vdupq_n_u64(~0ULL);
  uint64x2_t nanXY = veorq_u64(vceqq_f64(m_v.xy, m_v.xy), allOnes);
  uint64x2_t nanZW = veorq_u64(vceqq_f64(m_v.zw, m_v.zw), allOnes);
  int m1 = (vgetq_lane_u64(nanXY, 0) ? 1 : 0) | (vgetq_lane_u64(nanXY, 1) ? 2 : 0);
  int m2 = (vgetq_lane_u64(nanZW, 0) ? 1 : 0) | (vgetq_lane_u64(nanZW, 1) ? 2 : 0);
  return ((m1 | (m2 << 2)) & mask) != 0;
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsValid() const
{
  // Valid = not NaN and not Inf
  // Inf/NaN: exponent bits all 1
  uint64x2_t exponentMask = vdupq_n_u64(0x7FF0000000000000ULL);
  uint64x2_t allOnes = vdupq_n_u64(~0ULL);

  uint64x2_t bitsXY = vreinterpretq_u64_f64(m_v.xy);
  uint64x2_t bitsZW = vreinterpretq_u64_f64(m_v.zw);

  // Check if exponent is NOT all 1s
  uint64x2_t maskedXY = vandq_u64(bitsXY, exponentMask);
  uint64x2_t maskedZW = vandq_u64(bitsZW, exponentMask);
  uint64x2_t validXY = veorq_u64(vceqq_u64(maskedXY, exponentMask), allOnes);
  uint64x2_t validZW = veorq_u64(vceqq_u64(maskedZW, exponentMask), allOnes);

  const int mask = EZ_BIT(N) - 1;
  int m1 = (vgetq_lane_u64(validXY, 0) ? 1 : 0) | (vgetq_lane_u64(validXY, 1) ? 2 : 0);
  int m2 = (vgetq_lane_u64(validZW, 0) ? 1 : 0) | (vgetq_lane_u64(validZW, 1) ? 2 : 0);
  return ((m1 | (m2 << 2)) & mask) == mask;
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetComponent() const
{
  ezSimdDouble result;
  if constexpr (N == 0)
  {
    result.m_v.xy = vdupq_laneq_f64(m_v.xy, 0);
  }
  else if constexpr (N == 1)
  {
    result.m_v.xy = vdupq_laneq_f64(m_v.xy, 1);
  }
  else if constexpr (N == 2)
  {
    result.m_v.xy = vdupq_laneq_f64(m_v.zw, 0);
  }
  else
  {
    result.m_v.xy = vdupq_laneq_f64(m_v.zw, 1);
  }
  result.m_v.zw = result.m_v.xy;
  return result;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::x() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::y() const
{
  return GetComponent<1>();
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::z() const
{
  return GetComponent<2>();
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::w() const
{
  return GetComponent<3>();
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Get() const
{
  ezSimdVec4d result;

  alignas(16) double vals[4];
  vst1q_f64(vals, m_v.xy);
  vst1q_f64(vals + 2, m_v.zw);

  alignas(16) double out[4];
  out[0] = vals[(s & 0x3000) >> 12];
  out[1] = vals[(s & 0x0300) >> 8];
  out[2] = vals[(s & 0x0030) >> 4];
  out[3] = vals[(s & 0x0003)];

  result.m_v.xy = vld1q_f64(out);
  result.m_v.zw = vld1q_f64(out + 2);
  return result;
}

template <ezSwizzle::Enum s>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetCombined(const ezSimdVec4d& other) const
{
  ezSimdVec4d result;

  alignas(16) double vals[4];
  alignas(16) double otherVals[4];
  vst1q_f64(vals, m_v.xy);
  vst1q_f64(vals + 2, m_v.zw);
  vst1q_f64(otherVals, other.m_v.xy);
  vst1q_f64(otherVals + 2, other.m_v.zw);

  alignas(16) double out[4];
  out[0] = vals[(s & 0x3000) >> 12];
  out[1] = vals[(s & 0x0300) >> 8];
  out[2] = otherVals[(s & 0x0030) >> 4];
  out[3] = otherVals[(s & 0x0003)];

  result.m_v.xy = vld1q_f64(out);
  result.m_v.zw = vld1q_f64(out + 2);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator-() const
{
  ezSimdVec4d result;
  result.m_v.xy = vnegq_f64(m_v.xy);
  result.m_v.zw = vnegq_f64(m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator+(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = vaddq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vaddq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator-(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = vsubq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vsubq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator*(const ezSimdDouble& f) const
{
  ezSimdVec4d result;
  result.m_v.xy = vmulq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = vmulq_f64(m_v.zw, f.m_v.xy);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::operator/(const ezSimdDouble& f) const
{
  ezSimdVec4d result;
  result.m_v.xy = vdivq_f64(m_v.xy, f.m_v.xy);
  result.m_v.zw = vdivq_f64(m_v.zw, f.m_v.xy);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMul(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = vmulq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vmulq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompDiv(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = vdivq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vdivq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMin(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = vminq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vminq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CompMax(const ezSimdVec4d& v) const
{
  ezSimdVec4d result;
  result.m_v.xy = vmaxq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vmaxq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Abs() const
{
  ezSimdVec4d result;
  result.m_v.xy = vabsq_f64(m_v.xy);
  result.m_v.zw = vabsq_f64(m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Round() const
{
  ezSimdVec4d result;
  result.m_v.xy = vrndnq_f64(m_v.xy);
  result.m_v.zw = vrndnq_f64(m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Floor() const
{
  ezSimdVec4d result;
  result.m_v.xy = vrndmq_f64(m_v.xy);
  result.m_v.zw = vrndmq_f64(m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Ceil() const
{
  ezSimdVec4d result;
  result.m_v.xy = vrndpq_f64(m_v.xy);
  result.m_v.zw = vrndpq_f64(m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Trunc() const
{
  ezSimdVec4d result;
  result.m_v.xy = vrndq_f64(m_v.xy);
  result.m_v.zw = vrndq_f64(m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::FlipSign(const ezSimdVec4bWide& vCmp) const
{
  ezSimdVec4d result;
  // Create sign mask: -0.0 has bit pattern with only sign bit set
  float64x2_t signMask = vreinterpretq_f64_u64(vdupq_n_u64(0x8000000000000000ULL));
  // AND with comparison mask to get sign bit where comparison is true
  float64x2_t flipXY = vreinterpretq_f64_u64(vandq_u64(vCmp.m_v.xy, vreinterpretq_u64_f64(signMask)));
  float64x2_t flipZW = vreinterpretq_f64_u64(vandq_u64(vCmp.m_v.zw, vreinterpretq_u64_f64(signMask)));
  // XOR to flip sign
  result.m_v.xy = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(m_v.xy), vreinterpretq_u64_f64(flipXY)));
  result.m_v.zw = vreinterpretq_f64_u64(veorq_u64(vreinterpretq_u64_f64(m_v.zw), vreinterpretq_u64_f64(flipZW)));
  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Select(const ezSimdVec4bWide& vCmp, const ezSimdVec4d& vTrue, const ezSimdVec4d& vFalse)
{
  ezSimdVec4d result;
  result.m_v.xy = vbslq_f64(vCmp.m_v.xy, vTrue.m_v.xy, vFalse.m_v.xy);
  result.m_v.zw = vbslq_f64(vCmp.m_v.zw, vTrue.m_v.zw, vFalse.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator+=(const ezSimdVec4d& v)
{
  m_v.xy = vaddq_f64(m_v.xy, v.m_v.xy);
  m_v.zw = vaddq_f64(m_v.zw, v.m_v.zw);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator-=(const ezSimdVec4d& v)
{
  m_v.xy = vsubq_f64(m_v.xy, v.m_v.xy);
  m_v.zw = vsubq_f64(m_v.zw, v.m_v.zw);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator*=(const ezSimdDouble& f)
{
  m_v.xy = vmulq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = vmulq_f64(m_v.zw, f.m_v.xy);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4d& ezSimdVec4d::operator/=(const ezSimdDouble& f)
{
  m_v.xy = vdivq_f64(m_v.xy, f.m_v.xy);
  m_v.zw = vdivq_f64(m_v.zw, f.m_v.xy);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator==(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = vceqq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vceqq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator!=(const ezSimdVec4d& v) const
{
  return !(*this == v);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator<=(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = vcleq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vcleq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator<(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = vcltq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vcltq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator>=(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = vcgeq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vcgeq_f64(m_v.zw, v.m_v.zw);
  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::operator>(const ezSimdVec4d& v) const
{
  ezSimdVec4bWide result;
  result.m_v.xy = vcgtq_f64(m_v.xy, v.m_v.xy);
  result.m_v.zw = vcgtq_f64(m_v.zw, v.m_v.zw);
  return result;
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<2>() const
{
  // vpaddq_f64 does pairwise addition: [a0,a1], [b0,b1] -> [a0+a1, b0+b1]
  // Passing same vector twice: [x,y], [x,y] -> [x+y, x+y]
  float64x2_t sum = vpaddq_f64(m_v.xy, m_v.xy);
  return ezSimdDouble(vgetq_lane_f64(sum, 0));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<3>() const
{
  // First reduce xy pair, then add z
  float64x2_t sumXY = vpaddq_f64(m_v.xy, m_v.xy); // [x+y, x+y]
  double z = vgetq_lane_f64(m_v.zw, 0);
  return ezSimdDouble(vgetq_lane_f64(sumXY, 0) + z);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<4>() const
{
  // vpaddq_f64: [x,y], [z,w] -> [x+y, z+w]
  // Then pairwise add again to get final sum
  float64x2_t pair = vpaddq_f64(m_v.xy, m_v.zw); // [x+y, z+w]
  float64x2_t total = vpaddq_f64(pair, pair);    // [x+y+z+w, x+y+z+w]
  return ezSimdDouble(vgetq_lane_f64(total, 0));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<2>() const
{
  // vpminq_f64 does pairwise min: [a0,a1], [b0,b1] -> [min(a0,a1), min(b0,b1)]
  // Passing same vector twice: [x,y], [x,y] -> [min(x,y), min(x,y)]
  float64x2_t minVal = vpminq_f64(m_v.xy, m_v.xy);
  return ezSimdDouble(vgetq_lane_f64(minVal, 0));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<3>() const
{
  // First reduce xy pair, then compare with z
  float64x2_t minXY = vpminq_f64(m_v.xy, m_v.xy); // [min(x,y), min(x,y)]
  double z = vgetq_lane_f64(m_v.zw, 0);
  double minXYVal = vgetq_lane_f64(minXY, 0);
  return ezSimdDouble(minXYVal < z ? minXYVal : z);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<4>() const
{
  // vpminq_f64: [x,y], [z,w] -> [min(x,y), min(z,w)]
  // Then pairwise min again to get final minimum
  float64x2_t pair = vpminq_f64(m_v.xy, m_v.zw); // [min(x,y), min(z,w)]
  float64x2_t minAll = vpminq_f64(pair, pair);   // [min(x,y,z,w), min(x,y,z,w)]
  return ezSimdDouble(vgetq_lane_f64(minAll, 0));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<2>() const
{
  // vpmaxq_f64 does pairwise max: [a0,a1], [b0,b1] -> [max(a0,a1), max(b0,b1)]
  // Passing same vector twice: [x,y], [x,y] -> [max(x,y), max(x,y)]
  float64x2_t maxVal = vpmaxq_f64(m_v.xy, m_v.xy);
  return ezSimdDouble(vgetq_lane_f64(maxVal, 0));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<3>() const
{
  // First reduce xy pair, then compare with z
  float64x2_t maxXY = vpmaxq_f64(m_v.xy, m_v.xy); // [max(x,y), max(x,y)]
  double z = vgetq_lane_f64(m_v.zw, 0);
  double maxXYVal = vgetq_lane_f64(maxXY, 0);
  return ezSimdDouble(maxXYVal > z ? maxXYVal : z);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<4>() const
{
  // vpmaxq_f64: [x,y], [z,w] -> [max(x,y), max(z,w)]
  // Then pairwise max again to get final maximum
  float64x2_t pair = vpmaxq_f64(m_v.xy, m_v.zw); // [max(x,y), max(z,w)]
  float64x2_t maxAll = vpmaxq_f64(pair, pair);   // [max(x,y,z,w), max(x,y,z,w)]
  return ezSimdDouble(vgetq_lane_f64(maxAll, 0));
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<1>(const ezSimdVec4d& v) const
{
  return CompMul(v).GetComponent<0>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<2>(const ezSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<2>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<3>(const ezSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<3>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::Dot<4>(const ezSimdVec4d& v) const
{
  return CompMul(v).HorizontalSum<4>();
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CrossRH(const ezSimdVec4d& v) const
{
  // Cross product: (y1*z2 - z1*y2, z1*x2 - x1*z2, x1*y2 - y1*x2, 0)
  double x1 = vgetq_lane_f64(m_v.xy, 0);
  double y1 = vgetq_lane_f64(m_v.xy, 1);
  double z1 = vgetq_lane_f64(m_v.zw, 0);
  double x2 = vgetq_lane_f64(v.m_v.xy, 0);
  double y2 = vgetq_lane_f64(v.m_v.xy, 1);
  double z2 = vgetq_lane_f64(v.m_v.zw, 0);

  alignas(16) double result[4] = {
    y1 * z2 - z1 * y2,
    z1 * x2 - x1 * z2,
    x1 * y2 - y1 * x2,
    0.0};

  ezSimdVec4d r;
  r.m_v.xy = vld1q_f64(result);
  r.m_v.zw = vld1q_f64(result + 2);
  return r;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulAdd(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c)
{
  ezSimdVec4d result;
  result.m_v.xy = vfmaq_f64(c.m_v.xy, a.m_v.xy, b.m_v.xy);
  result.m_v.zw = vfmaq_f64(c.m_v.zw, a.m_v.zw, b.m_v.zw);
  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulAdd(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c)
{
  ezSimdVec4d result;
  result.m_v.xy = vfmaq_f64(c.m_v.xy, a.m_v.xy, b.m_v.xy);
  result.m_v.zw = vfmaq_f64(c.m_v.zw, a.m_v.zw, b.m_v.xy);
  return result;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulSub(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& c)
{
  // MulSub should compute a*b - c
  return a.CompMul(b) - c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MulSub(const ezSimdVec4d& a, const ezSimdDouble& b, const ezSimdVec4d& c)
{
  // MulSub should compute a*b - c
  return a * b - c;
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::CopySign(const ezSimdVec4d& vMagnitude, const ezSimdVec4d& vSign)
{
  // Returns values with the magnitude of vMagnitude and the sign of vSign.
  // IEEE 754 double: bit 63 = sign, bits 62-52 = exponent, bits 51-0 = mantissa
  ezSimdVec4d result;

  // signMask: only bit 63 set (the sign bit)
  uint64x2_t signMask = vdupq_n_u64(0x8000000000000000ULL);
  // valueMask: all bits except bit 63 (magnitude bits)
  uint64x2_t valueMask = vdupq_n_u64(0x7FFFFFFFFFFFFFFFULL);

  // Extract magnitude bits (clear sign bit) from vMagnitude
  uint64x2_t magBitsXY = vandq_u64(vreinterpretq_u64_f64(vMagnitude.m_v.xy), valueMask);
  uint64x2_t magBitsZW = vandq_u64(vreinterpretq_u64_f64(vMagnitude.m_v.zw), valueMask);

  // Extract only the sign bit from vSign
  uint64x2_t signBitsXY = vandq_u64(vreinterpretq_u64_f64(vSign.m_v.xy), signMask);
  uint64x2_t signBitsZW = vandq_u64(vreinterpretq_u64_f64(vSign.m_v.zw), signMask);

  // Combine magnitude with new sign using OR
  result.m_v.xy = vreinterpretq_f64_u64(vorrq_u64(magBitsXY, signBitsXY));
  result.m_v.zw = vreinterpretq_f64_u64(vorrq_u64(magBitsZW, signBitsZW));
  return result;
}
