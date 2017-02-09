#pragma once

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f()
{
  EZ_CHECK_ALIGNMENT_16(this);

#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = _mm_set1_ps(ezMath::BasicType<float>::GetNaN());
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(float xyzw)
{
  EZ_CHECK_ALIGNMENT_16(this);

  m_v = _mm_set1_ps(xyzw);
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(const ezSimdFloat& xyzw)
{
  EZ_CHECK_ALIGNMENT_16(this);

  m_v = xyzw.m_v;
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(float x, float y, float z, float w)
{
  EZ_CHECK_ALIGNMENT_16(this);

  m_v = _mm_setr_ps(x, y, z, w);
}

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(ezInternal::QuadFloat v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float xyzw)
{
  m_v = _mm_set1_ps(xyzw);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::Set(float x, float y, float z, float w)
{
  m_v = _mm_setr_ps(x, y, z, w);
}

EZ_ALWAYS_INLINE void ezSimdVec4f::SetZero()
{
  m_v = _mm_setzero_ps();
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<1>(const float* pFloat)
{
  m_v = _mm_load_ss(pFloat);
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<2>(const float* pFloat)
{
  m_v = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pFloat)));
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<3>(const float* pFloat)
{
  m_v = _mm_movelh_ps(_mm_castpd_ps(_mm_load_sd(reinterpret_cast<const double*>(pFloat))), _mm_load_ss(pFloat + 2));
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Load<4>(const float* pFloat)
{
  m_v = _mm_loadu_ps(pFloat);
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<1>(float* pFloat) const
{
  _mm_store_ss(pFloat, m_v);
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<2>(float* pFloat) const
{
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(m_v));
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<3>(float* pFloat) const
{
  _mm_store_sd(reinterpret_cast<double*>(pFloat), _mm_castps_pd(m_v));
  _mm_store_ss(pFloat + 2, _mm_movehl_ps(m_v, m_v));
}

template<>
EZ_ALWAYS_INLINE void ezSimdVec4f::Store<4>(float* pFloat) const
{
  _mm_storeu_ps(pFloat, m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetReciprocal<ezMathAcc::BITS_12>() const
{
  return _mm_rcp_ps(m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetReciprocal<ezMathAcc::BITS_23>() const
{
  __m128 x0 = _mm_rcp_ps(m_v);

  // One Newton-Raphson iteration
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(m_v, x0)));

  return x1;
}

template<>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetReciprocal<ezMathAcc::FULL>() const
{
  return _mm_div_ps(_mm_set1_ps(1.0f), m_v);
}

template<int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetLength() const
{
  return GetLengthSquared<N>().GetSqrt<acc>();
}

template<int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetInvLength() const
{
  return GetLengthSquared<N>().GetInvSqrt<acc>();
}

template<int N>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template<int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetLengthAndNormalize()
{
  const ezSimdFloat squaredLen = GetLengthSquared<N>();
  const ezSimdFloat reciprocalLen = squaredLen.GetInvSqrt<acc>();
  *this = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template<int N, ezMathAcc::Enum acc>
ezSimdVec4f ezSimdVec4f::GetNormalized() const
{
  // Cannot partially specialize function templates - delegate to a helper class
  template<ezMathAcc::acc> struct Helper {};

  template<> struct Helper<ezMathAcc::FULL>
  {
    ezSimdVec4f operator()(const ezSimdVec4f& self)
    {
      return self / self.GetLength<N, ezMathAcc::FULL>();
    }
  };

  template<> struct Helper<ezMathAcc::BITS_23>
  {
    ezSimdVec4f operator()(const ezSimdVec4f& self)
    {
      return self * self.GetInvLength<N, ezMathAcc::BITS_23>();
    }
  };

  template<> struct Helper<ezMathAcc::BITS_12>
  {
    ezSimdVec4f operator()(const ezSimdVec4f& self)
    {
      return self * self.GetInvLength<N, ezMathAcc::BITS_12>();
    }
  };

  return Helper<acc>()(*this);
}

template<int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE void ezSimdVec4f::Normalize()
{
  *this = GetNormalized<N, acc>();
}

template<int N, ezMathAcc::Enum acc>
ezResult ezSimdVec4f::NormalizeIfNotZero(const ezSimdVec4f& vFallback, const ezSimdFloat& fEpsilon)
{
  ezSimdFloat sqLength = GetLengthSquared<N>();

  if(sqLength < fEpsilon)
  {
    *this = vFallback;
    return EZ_FAILURE;
  }
  else
  {
    *this *= sqLength.GetInvSqrt<acc>();
    return EZ_SUCCESS;
  }
}

template<int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsZero() const
{
  return _mm_movemask_ps(_mm_cmpeq_ps(m_v, _mm_setzero_ps())) == ((1 << N) - 1);
}

template<int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsNormalized(const ezSimdFloat& fEpsilon) const
{
  const ezSimdFloat sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0f, fEpsilon);
}

template<int N>
inline bool ezSimdVec4f::IsNaN() const
{
  // NAN -> (exponent = all 1, mantissa = non-zero)

  const ezUInt32 s_exponentMask[4] = { 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000 };
  const ezUInt32 s_mantissaMask[4] = { 0x7FFFFF, 0x7FFFFF, 0x7FFFFF, 0x7FFFFF };

  __m128 exponentMask = _mm_loadu_ps(reinterpret_cast<const float*>(s_exponentMask));
  __m128 mantissaMask = _mm_loadu_ps(reinterpret_cast<const float*>(s_mantissaMask));

  __m128 exponentAll1 = _mm_cmpeq_ps(_mm_and_ps(m_v, exponentMask), exponentMask);
  __m128 mantissaNon0 = _mm_cmpneq_ps(_mm_and_ps(m_v, mantissaMask), _mm_setzero_ps());

  return _mm_movemask_ps(_mm_and_ps(exponentAll1, mantissaNon0)) == ((1 << N) - 1);
}

template<int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsValid() const
{
  // Check the 8 exponent bits.
  // NAN -> (exponent = all 1, mantissa = non-zero)
  // INF -> (exponent = all 1, mantissa = zero)

  const ezUInt32 s_exponentMask[4] = { 0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000 };

  __m128 exponentMask = _mm_loadu_ps(reinterpret_cast<const float*>(s_exponentMask));

  __m128 exponentNot1 = _mm_cmpneq_ps(_mm_and_ps(m_v, exponentMask), exponentMask);

  return _mm_movemask_ps(exponentNot1) == ((1 << N) - 1);
}

template<int N>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetComponent() const
{
  return _mm_shuffle_ps(m_v, m_v, _MM_SHUFFLE(N, N, N, N));
}

ezSimdFloat ezSimdVec4f::GetComponent(int i) const
{
  switch(i)
  {
  case 0:
    return GetComponent<0>();

  case 1:
    return GetComponent<1>();

  case 2:
    return GetComponent<2>();

  default:
    return GetComponent<3>();
  }
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
  const int shuffle = ((s >> 12) & 0x03) | ((s >> 6) & 0x0c) | (s & 0x30) | ((s << 6) & 0xc0);
  return _mm_shuffle_ps(m_v, m_v, shuffle);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator+(const ezSimdVec4f& v) const
{
  return _mm_add_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator-(const ezSimdVec4f& v) const
{
  return _mm_sub_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator*(const ezSimdFloat& f) const
{
  return _mm_mul_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::operator / (const ezSimdFloat& f) const
{
  return _mm_div_ps(m_v, f.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMul(const ezSimdVec4f& v) const
{
  return _mm_mul_ps(m_v, v.m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompDiv<ezMathAcc::FULL>(const ezSimdVec4f& v) const
{
  return _mm_div_ps(m_v, v.m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompDiv<ezMathAcc::BITS_23>(const ezSimdVec4f& v) const
{
  __m128 x0 = _mm_rcp_ps(v.m_v);

  // One iteration of Newton-Raphson
  __m128 x1 = _mm_mul_ps(x0, _mm_sub_ps(_mm_set1_ps(2.0f), _mm_mul_ps(v.m_v, x0)));

  return _mm_mul_ps(m_v, x1);
}

template<>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompDiv<ezMathAcc::BITS_12>(const ezSimdVec4f& v) const
{
  return _mm_mul_ps(m_v, _mm_rcp_ps(v.m_v));
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMin(const ezSimdVec4f& v) const
{
  return _mm_min_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CompMax(const ezSimdVec4f& v) const
{
  return _mm_max_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Abs() const
{
  return _mm_andnot_ps(_mm_set1_ps(-0.0f), m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::FlipSign(const ezSimdVec4b& cmp) const
{
  return _mm_xor_ps(m_v, _mm_and_ps(cmp.m_v, _mm_set1_ps(-0.0f)));
}

//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Select(const ezSimdVec4f& ifFalse, const ezSimdVec4f& ifTrue, const ezSimdVec4b& cmp)
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_blendv_ps(ifFalse.m_v, ifTrue.m_v, cmp.m_v);
#else
  return _mm_or_ps(_mm_andnot_ps(cmp.m_v, ifFalse.m_v), _mm_and_ps(cmp.m_v, ifTrue.m_v));
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator+=(const ezSimdVec4f& v)
{
  m_v = _mm_add_ps(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator-=(const ezSimdVec4f& v)
{
  m_v = _mm_sub_ps(m_v, v.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator*=(const ezSimdFloat& f)
{
  m_v = _mm_mul_ps(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4f& ezSimdVec4f::operator/=(const ezSimdFloat& f)
{
  m_v = _mm_div_ps(m_v, f.m_v);
  return *this;
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::IsEqual(const ezSimdVec4f& rhs, const ezSimdFloat& fEpsilon) const
{
  ezSimdVec4f minusEps = rhs - ezSimdVec4f(fEpsilon);
  ezSimdVec4f plusEps = rhs + ezSimdVec4f(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator==(const ezSimdVec4f& v) const
{
  return _mm_cmpeq_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator!=(const ezSimdVec4f& v) const
{
  return _mm_cmpneq_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator<=(const ezSimdVec4f& v) const
{
  return _mm_cmple_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator<(const ezSimdVec4f& v) const
{
  return _mm_cmplt_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator>=(const ezSimdVec4f& v) const
{
  return _mm_cmpge_ps(m_v, v.m_v);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::operator>(const ezSimdVec4f& v) const
{
  return _mm_cmpgt_ps(m_v, v.m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<2>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_31
  __m128 a = _mm_hadd_ps(m_v, m_v);
  return _mm_shuffle_ps(a, a, _MM_SHUFFLE(0, 0, 0, 0));
#else
  return GetComponent<0>() + GetComponent<1>();
#endif
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<3>() const
{
  return HorizontalSum<2>() + GetComponent<2>();
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<4>() const
{
#if EZ_SSE_LEVEL >= EZ_SSE_31
  __m128 a = _mm_hadd_ps(m_v, m_v);
  return _mm_hadd_ps(a, a);
#else
  return (GetComponent<0>() + GetComponent<1>()) + (GetComponent<2>() + GetComponent<3>());
#endif
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<2>() const
{
  return _mm_min_ps(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<3>() const
{
  return _mm_min_ps(_mm_min_ps(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<4>() const
{
  __m128 xyxyzwzw = _mm_min_ps(_mm_shuffle_ps(m_v, m_v, _MM_SHUFFLE(1, 0, 3, 2)), m_v);
  __m128 zwzwxyxy = _mm_shuffle_ps(xyxyzwzw, xyxyzwzw, _MM_SHUFFLE(2, 3, 0, 1));
  return _mm_min_ps(xyxyzwzw, zwzwxyxy);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<2>() const
{
  return _mm_max_ps(GetComponent<0>().m_v, GetComponent<1>().m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<3>() const
{
  return _mm_max_ps(_mm_max_ps(GetComponent<0>().m_v, GetComponent<1>().m_v), GetComponent<2>().m_v);
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<4>() const
{
  __m128 xyxyzwzw = _mm_max_ps(_mm_shuffle_ps(m_v, m_v, _MM_SHUFFLE(1, 0, 3, 2)), m_v);
  __m128 zwzwxyxy = _mm_shuffle_ps(xyxyzwzw, xyxyzwzw, _MM_SHUFFLE(2, 3, 0, 1));
  return _mm_max_ps(xyxyzwzw, zwzwxyxy);
}


template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot<2>(const ezSimdVec4f& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x3f);
#else
  return CompMult(v).HorizontalSum<2>();
#endif
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot<3>(const ezSimdVec4f& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0x7f);
#else
  return CompMult(v).HorizontalSum<3>();
#endif
}

template<>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::Dot<4>(const ezSimdVec4f& v) const
{
#if EZ_SSE_LEVEL >= EZ_SSE_41
  return _mm_dp_ps(m_v, v.m_v, 0xff);
#else
  return CompMult(v).HorizontalSum<4>();
#endif
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Cross(const ezSimdVec4f& v) const
{
  __m128 a = _mm_mul_ps(m_v, _mm_shuffle_ps(v.m_v, v.m_v, _MM_SHUFFLE(3, 0, 2, 1)));
  __m128 b = _mm_mul_ps(v.m_v, _mm_shuffle_ps(m_v, m_v, _MM_SHUFFLE(3, 0, 2, 1)));
  __m128 c = _mm_sub_ps(a, b);

  return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetOrthogonalVector() const
{
  // See http://blog.selfshadow.com/2011/10/17/perp-vectors/ - this is Stark's first variant, SIMDified.
  return Cross(_mm_cmpeq_ps(m_v, HorizontalMin<3>().m_v));
}

//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::ZeroVector()
{
  return _mm_setzero_ps();
}

//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulAdd(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm_fmadd_ps(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) + c;
#endif
}

//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulAdd(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm_fmadd_ps(a.m_v, b.m_v, c.m_v);
#else
  return a * b + c;
#endif
}

//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulSub(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm_fmsub_ps(a.m_v, b.m_v, c.m_v);
#else
  return a.CompMul(b) - c;
#endif
}

//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::MulSub(const ezSimdVec4f& a, const ezSimdFloat& b, const ezSimdVec4f& c)
{
#if EZ_SSE_LEVEL >= EZ_SSE_AVX2
  return _mm_fmsub_ps(a.m_v, b.m_v, c.m_v);
#else
  return a * b - c;
#endif
}

//static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::CopySign(const ezSimdVec4f& magnitude, const ezSimdVec4f& sign)
{
  __m128 minusZero = _mm_set1_ps(-0.0f);
  return _mm_or_ps(_mm_andnot_ps(minusZero, magnitude.m_v), _mm_and_ps(minusZero, sign.m_v));
}
