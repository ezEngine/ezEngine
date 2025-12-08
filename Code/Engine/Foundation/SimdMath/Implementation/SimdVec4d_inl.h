#pragma once



EZ_ALWAYS_INLINE ezSimdVec4d::ezSimdVec4d(ezInternal::QuadDouble v)
{
  m_v = v;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MakeZero()
{
  return ezSimdVec4d(ezSimdDouble::MakeZero());
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::MakeNaN()
{
  return ezSimdVec4d(ezSimdDouble::MakeNaN());
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetLength() const
{
  const ezSimdDouble squaredLen = GetLengthSquared<N>();
  return squaredLen.GetSqrt();
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetInvLength() const
{
  const ezSimdDouble squaredLen = GetLengthSquared<N>();
  return squaredLen.GetInvSqrt();
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetLengthSquared() const
{
  return Dot<N>(*this);
}

template <int N>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::GetLengthAndNormalize()
{
  const ezSimdDouble squaredLen = GetLengthSquared<N>();
  const ezSimdDouble reciprocalLen = squaredLen.GetInvSqrt();
  *this = (*this) * reciprocalLen;
  return squaredLen * reciprocalLen;
}

template <int N>
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetNormalized() const
{
  return (*this) * GetInvLength<N>();
}

template <int N>
EZ_ALWAYS_INLINE void ezSimdVec4d::Normalize()
{
  *this = GetNormalized<N>();
}

template <int N>
EZ_ALWAYS_INLINE void ezSimdVec4d::NormalizeIfNotZero(const ezSimdVec4d& vFallback, const ezSimdDouble& fEpsilon)
{
  ezSimdVec4bWide bIsZero(IsZero<N>(fEpsilon));
  *this = Select(bIsZero, vFallback, GetNormalized<N>());
}

template <int N>
EZ_ALWAYS_INLINE bool ezSimdVec4d::IsNormalized(const ezSimdDouble& fEpsilon) const
{
  const ezSimdDouble sqLength = GetLengthSquared<N>();
  return sqLength.IsEqual(1.0, fEpsilon);
}

inline ezSimdDouble ezSimdVec4d::GetComponent(int i) const
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

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Fraction() const
{
  return *this - Trunc();
}

// static
EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::Lerp(const ezSimdVec4d& a, const ezSimdVec4d& b, const ezSimdVec4d& t)
{
  return a + t.CompMul(b - a);
}

EZ_ALWAYS_INLINE ezSimdVec4bWide ezSimdVec4d::IsEqual(const ezSimdVec4d& rhs, const ezSimdDouble& fEpsilon) const
{
  ezSimdVec4d minusEps = rhs - ezSimdVec4d(fEpsilon);
  ezSimdVec4d plusEps = rhs + ezSimdVec4d(fEpsilon);
  return (*this >= minusEps) && (*this <= plusEps);
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalSum<1>() const
{
  return GetComponent<0>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMin<1>() const
{
  return GetComponent<0>();
}

template <>
EZ_ALWAYS_INLINE ezSimdDouble ezSimdVec4d::HorizontalMax<1>() const
{
  return GetComponent<0>();
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdVec4d::GetOrthogonalVector() const
{
  const ezSimdVec4bWide bIsLessThan = Get<ezSwizzle::YYYY>() < ezSimdVec4d(0.99);
  return CrossRH(Select(bIsLessThan, ezSimdVec4d(0.0, 1.0, 0.0, 0.0), ezSimdVec4d(1.0, 0.0, 0.0, 0.0)));
}

EZ_ALWAYS_INLINE const ezSimdVec4d operator*(const ezSimdDouble& f, const ezSimdVec4d& v)
{
  return v * f;
}
