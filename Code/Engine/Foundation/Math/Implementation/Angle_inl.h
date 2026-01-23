#pragma once

template <typename Type>
constexpr EZ_ALWAYS_INLINE Type ezAngleTemplate<Type>::Pi()
{
  return static_cast<Type>(3.1415926535897932384626433832795);
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE Type ezAngleTemplate<Type>::DegToRadMultiplier()
{
  return Pi() / (Type)180;
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE Type ezAngleTemplate<Type>::RadToDegMultiplier()
{
  return ((Type)180) / Pi();
}

template <typename Type>
constexpr Type ezAngleTemplate<Type>::DegToRad(Type f)
{
  return f * DegToRadMultiplier();
}

template <typename Type>
constexpr Type ezAngleTemplate<Type>::RadToDeg(Type f)
{
  return f * RadToDegMultiplier();
}

template <typename Type>
constexpr inline ezAngleTemplate<Type> ezAngleTemplate<Type>::MakeFromDegree(Type fDegree)
{
  return ezAngleTemplate<Type>(DegToRad(fDegree));
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE ezAngleTemplate<Type> ezAngleTemplate<Type>::MakeFromRadian(Type fRadian)
{
  return ezAngleTemplate<Type>(fRadian);
}

template <typename Type>
constexpr inline Type ezAngleTemplate<Type>::GetDegree() const
{
  return RadToDeg(m_fRadian);
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE Type ezAngleTemplate<Type>::GetRadian() const
{
  return m_fRadian;
}

template <typename Type>
inline ezAngleTemplate<Type> ezAngleTemplate<Type>::GetNormalizedRange() const
{
  ezAngleTemplate<Type> out(m_fRadian);
  out.NormalizeRange();
  return out;
}

template <typename Type>
inline bool ezAngleTemplate<Type>::IsEqualSimple(ezAngleTemplate<Type> rhs, ezAngleTemplate<Type> epsilon) const
{
  const ezAngleTemplate<Type> diff = AngleBetween(*this, rhs);

  return ((diff.m_fRadian >= -epsilon.m_fRadian) && (diff.m_fRadian <= epsilon.m_fRadian));
}

template <typename Type>
inline bool ezAngleTemplate<Type>::IsEqualNormalized(ezAngleTemplate<Type> rhs, ezAngleTemplate<Type> epsilon) const
{
  // equality between normalized angles
  const ezAngleTemplate<Type> aNorm = GetNormalizedRange();
  const ezAngleTemplate<Type> bNorm = rhs.GetNormalizedRange();

  return aNorm.IsEqualSimple(bNorm, epsilon);
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE ezAngleTemplate<Type> ezAngleTemplate<Type>::operator-() const
{
  return ezAngleTemplate<Type>(-m_fRadian);
}

template <typename Type>
EZ_ALWAYS_INLINE void ezAngleTemplate<Type>::operator+=(ezAngleTemplate<Type> r)
{
  m_fRadian += r.m_fRadian;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezAngleTemplate<Type>::operator-=(ezAngleTemplate<Type> r)
{
  m_fRadian -= r.m_fRadian;
}

template <typename Type>
constexpr inline ezAngleTemplate<Type> ezAngleTemplate<Type>::operator+(ezAngleTemplate<Type> r) const
{
  return ezAngleTemplate<Type>(m_fRadian + r.m_fRadian);
}

template <typename Type>
constexpr inline ezAngleTemplate<Type> ezAngleTemplate<Type>::operator-(ezAngleTemplate<Type> r) const
{
  return ezAngleTemplate<Type>(m_fRadian - r.m_fRadian);
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE bool ezAngleTemplate<Type>::operator==(const ezAngleTemplate<Type>& r) const
{
  return m_fRadian == r.m_fRadian;
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE bool ezAngleTemplate<Type>::operator!=(const ezAngleTemplate<Type>& r) const
{
  return m_fRadian != r.m_fRadian;
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE bool ezAngleTemplate<Type>::operator<(const ezAngleTemplate<Type>& r) const
{
  return m_fRadian < r.m_fRadian;
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE bool ezAngleTemplate<Type>::operator>(const ezAngleTemplate<Type>& r) const
{
  return m_fRadian > r.m_fRadian;
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE bool ezAngleTemplate<Type>::operator<=(const ezAngleTemplate<Type>& r) const
{
  return m_fRadian <= r.m_fRadian;
}

template <typename Type>
constexpr EZ_ALWAYS_INLINE bool ezAngleTemplate<Type>::operator>=(const ezAngleTemplate<Type>& r) const
{
  return m_fRadian >= r.m_fRadian;
}

template <typename Type>
constexpr inline ezAngleTemplate<Type> operator*(const ezAngleTemplate<Type>& a, Type f)
{
  return ezAngleTemplate<Type>::MakeFromRadian(a.GetRadian() * f);
}

template <typename Type>
constexpr inline ezAngleTemplate<Type> operator*(Type f, const ezAngleTemplate<Type>& a)
{
  return ezAngleTemplate<Type>::MakeFromRadian(a.GetRadian() * f);
}

template <typename Type>
constexpr inline ezAngleTemplate<Type> operator/(const ezAngleTemplate<Type>& a, Type f)
{
  return ezAngleTemplate<Type>::MakeFromRadian(a.GetRadian() / f);
}

template <typename Type>
constexpr inline Type operator/(const ezAngleTemplate<Type>& a, const ezAngleTemplate<Type>& b)
{
  return a.GetRadian() / b.GetRadian();
}

template <typename Type>
constexpr inline ezAngleTemplate<Type> ezAngleTemplate<Type>::AngleBetween(ezAngleTemplate<Type> a, ezAngleTemplate<Type> b)
{
  // taken from http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference

  return ezAngleTemplate<Type>(Pi() - ezMath::Abs(ezMath::Abs(a.GetRadian() - b.GetRadian()) - Pi()));
}



template <typename Type>
inline void ezAngleTemplate<Type>::NormalizeRange()
{
  constexpr Type fTwoPi = Type(2.0) * Pi();
  constexpr Type fTwoPiTen = Type(10.0) * Pi();

  if (m_fRadian > fTwoPiTen || m_fRadian < -fTwoPiTen)
  {
    m_fRadian = ezMath::Mod(m_fRadian, fTwoPi);
  }

  while (m_fRadian >= fTwoPi)
  {
    m_fRadian -= fTwoPi;
  }

  while (m_fRadian < Type(0.0))
  {
    m_fRadian += fTwoPi;
  }
}
