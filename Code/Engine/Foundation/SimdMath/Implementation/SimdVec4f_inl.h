#pragma once

EZ_ALWAYS_INLINE ezSimdVec4f::ezSimdVec4f(ezInternal::QuadFloat v)
{
  m_v = v;
}

template <int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetLength() const
{
  const ezSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetSqrt<acc>();
}

template <int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetInvLength() const
{
  const ezSimdFloat squaredLen = GetLengthSquared<N>();
  return squaredLen.GetInvSqrt<acc>();
}

template <int N>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template <int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::GetLengthAndNormalize()
{
  const ezSimdFloat squaredLen = GetLengthSquared<N>();
  const ezSimdFloat reciprocalLen = squaredLen.GetInvSqrt<acc>();
  *this = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template <int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::GetNormalized() const
{
  return (*this) * GetInvLength<N, acc>();
}

template <int N, ezMathAcc::Enum acc>
EZ_ALWAYS_INLINE void ezSimdVec4f::Normalize()
{
  *this = GetNormalized<N, acc>();
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4f::IsNormalized(const ezSimdFloat& fEpsilon) const
{
  const ezSimdFloat sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0f, fEpsilon);
}

inline ezSimdFloat ezSimdVec4f::GetComponent(int i) const
{
  switch (i)
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

// static
EZ_ALWAYS_INLINE ezSimdVec4f ezSimdVec4f::Lerp(const ezSimdVec4f& a, const ezSimdVec4f& b, const ezSimdVec4f& t)
{
  return a + t.CompMul(b - a);
}

EZ_ALWAYS_INLINE ezSimdVec4b ezSimdVec4f::IsEqual(const ezSimdVec4f& rhs, const ezSimdFloat& fEpsilon) const
{
  ezSimdVec4f minusEps = rhs - ezSimdVec4f(fEpsilon);
  ezSimdVec4f plusEps = rhs + ezSimdVec4f(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalSum<1>() const
{
  return GetComponent<0>();
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMin<1>() const
{
  return GetComponent<0>();
}

template <>
EZ_ALWAYS_INLINE ezSimdFloat ezSimdVec4f::HorizontalMax<1>() const
{
  return GetComponent<0>();
}
