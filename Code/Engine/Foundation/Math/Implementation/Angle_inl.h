#pragma once

template<typename Type>
constexpr EZ_ALWAYS_INLINE Type ezAngle::Pi()
{
  return static_cast<Type>(3.1415926535897932384626433832795);
}

template<typename Type>
constexpr EZ_ALWAYS_INLINE Type ezAngle::DegToRadMultiplier()
{
  return Pi<Type>() / (Type) 180;
}

template<typename Type>
constexpr EZ_ALWAYS_INLINE Type ezAngle::RadToDegMultiplier()
{
  return ((Type) 180) / Pi<Type>();
}

template<typename Type>
constexpr Type ezAngle::DegToRad(Type f)
{
  return f * DegToRadMultiplier<Type>();
}

template<typename Type>
constexpr Type ezAngle::RadToDeg(Type f)
{
  return f * RadToDegMultiplier<Type>();
}

constexpr inline ezAngle ezAngle::Degree(float fDegree)
{
  return ezAngle(DegToRad(fDegree));
}

constexpr EZ_ALWAYS_INLINE ezAngle ezAngle::Radian(float fRadian)
{
  return ezAngle(fRadian);
}

constexpr inline float ezAngle::GetDegree() const
{
  return RadToDeg(m_fRadian);
}

constexpr EZ_ALWAYS_INLINE float ezAngle::GetRadian() const
{
  return m_fRadian;
}

inline ezAngle ezAngle::GetNormalizedRange() const
{
  ezAngle out(m_fRadian);
  out.NormalizeRange();
  return out;
}

inline bool ezAngle::IsEqualSimple(ezAngle rhs, ezAngle epsilon) const
{
  const ezAngle diff = AngleBetween(*this, rhs);

  return ((diff.m_fRadian >= -epsilon.m_fRadian) && (diff.m_fRadian <= epsilon.m_fRadian));
}

inline bool ezAngle::IsEqualNormalized(ezAngle rhs, ezAngle epsilon) const
{
  // equality between normalized angles
  const ezAngle aNorm = GetNormalizedRange();
  const ezAngle bNorm = rhs.GetNormalizedRange();

  return aNorm.IsEqualSimple(bNorm, epsilon);
}

constexpr EZ_ALWAYS_INLINE ezAngle ezAngle::operator - () const
{
  return ezAngle(-m_fRadian);
}

EZ_ALWAYS_INLINE void ezAngle::operator += (ezAngle r)
{
  m_fRadian += r.m_fRadian;
}

EZ_ALWAYS_INLINE void ezAngle::operator -= (ezAngle r)
{
  m_fRadian -= r.m_fRadian;
}

constexpr inline ezAngle ezAngle::operator + (ezAngle r) const
{
  return ezAngle(m_fRadian + r.m_fRadian);
}

constexpr inline ezAngle ezAngle::operator - (ezAngle r) const
{
  return ezAngle(m_fRadian - r.m_fRadian);
}

constexpr EZ_ALWAYS_INLINE bool ezAngle::operator == (const ezAngle& r) const
{
  return m_fRadian == r.m_fRadian;
}

constexpr EZ_ALWAYS_INLINE bool ezAngle::operator != (const ezAngle& r) const
{
  return m_fRadian != r.m_fRadian;
}

constexpr EZ_ALWAYS_INLINE bool ezAngle::operator< (const ezAngle& r) const
{
  return m_fRadian < r.m_fRadian;
}

constexpr EZ_ALWAYS_INLINE bool ezAngle::operator> (const ezAngle& r) const
{
  return m_fRadian > r.m_fRadian;
}

constexpr inline ezAngle operator* (ezAngle a, float f)
{
  return ezAngle::Radian(a.GetRadian() * f);
}

constexpr inline ezAngle operator* (float f, ezAngle a)
{
  return ezAngle::Radian(a.GetRadian() * f);
}

constexpr inline ezAngle operator/ (ezAngle a, float f)
{
  return ezAngle::Radian(a.GetRadian() / f);
}

constexpr inline ezAngle operator/ (float f, ezAngle a)
{
  return ezAngle::Radian(a.GetRadian() / f);
}

