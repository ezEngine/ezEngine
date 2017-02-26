#pragma once

EZ_ALWAYS_INLINE ezSimdQuat::ezSimdQuat()
{

}

EZ_ALWAYS_INLINE ezSimdQuat::ezSimdQuat(const ezSimdVec4f& v)
{
  m_v = v;
}

//static
EZ_ALWAYS_INLINE ezSimdQuat ezSimdQuat::Identity()
{
  return ezSimdQuat(ezSimdVec4f(0.0f, 0.0f, 0.0f, 1.0f));
}

EZ_ALWAYS_INLINE void ezSimdQuat::SetIdentity()
{
  m_v.Set(0.0f, 0.0f, 0.0f, 1.0f);
}

EZ_ALWAYS_INLINE void ezSimdQuat::SetFromAxisAndAngle(const ezSimdVec4f& vRotationAxis, const ezSimdFloat& angle)
{
  ///\todo optimize
  const ezAngle halfAngle = ezAngle::Radian(angle) * 0.5f;
  float s = ezMath::Sin(halfAngle);
  float c = ezMath::Cos(halfAngle);

  m_v = vRotationAxis * s;
  m_v.SetW(c);
}

EZ_ALWAYS_INLINE void ezSimdQuat::Normalize()
{
  m_v.Normalize<4>();
}

inline ezResult ezSimdQuat::GetRotationAxisAndAngle(ezSimdVec4f& vAxis, ezSimdFloat& angle) const
{
  ///\todo optimize
  const ezAngle acos = ezMath::ACos(m_v.w());
  const float d = ezMath::Sin(acos);

  if (d == 0)
    return EZ_FAILURE;

  vAxis = m_v / d;
  angle = acos * 2;

  return EZ_SUCCESS;
}

EZ_ALWAYS_INLINE bool ezSimdQuat::IsValid(const ezSimdFloat& fEpsilon) const
{
  return m_v.IsNormalized<4>(fEpsilon);
}

EZ_ALWAYS_INLINE bool ezSimdQuat::IsNaN() const
{
  return m_v.IsNaN<4>();
}

EZ_ALWAYS_INLINE ezSimdQuat ezSimdQuat::operator-() const
{
  return m_v.FlipSign(ezSimdVec4b(true, true, true, false));
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdQuat::operator*(const ezSimdVec4f& v) const
{
  ezSimdVec4f t = m_v.Cross(v);
  t += t;
  return v + t * m_v.w() + m_v.Cross(t);
}

EZ_ALWAYS_INLINE ezSimdQuat ezSimdQuat::operator*(const ezSimdQuat& q2) const
{
  ezSimdQuat q;

  q.m_v = q2.m_v * m_v.w() + m_v * q2.m_v.w() + m_v.Cross(q2.m_v);
  q.m_v.SetW(m_v.w() * q2.m_v.w() - m_v.Dot<3>(q2.m_v));

  return q;
}

EZ_ALWAYS_INLINE bool ezSimdQuat::operator==(const ezSimdQuat& q2) const
{
  return (m_v == q2.m_v).AllSet<4>();
}

EZ_ALWAYS_INLINE bool ezSimdQuat::operator!=(const ezSimdQuat& q2) const
{
  return (m_v != q2.m_v).AnySet<4>();
}
