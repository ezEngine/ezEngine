#pragma once

EZ_ALWAYS_INLINE ezSimdTransform::ezSimdTransform()
{

}

EZ_ALWAYS_INLINE ezSimdTransform::ezSimdTransform(const ezSimdVec4f& position, const ezSimdQuat& rotation, const ezSimdVec4f& scale)
{
  m_Position = position;
  m_Rotation = rotation;
  m_Scale = scale;
}

EZ_ALWAYS_INLINE ezSimdTransform::ezSimdTransform(const ezSimdQuat& rotation)
{
  m_Position.SetZero();
  m_Rotation = rotation;
  m_Scale.Set(1.0f);
}

EZ_ALWAYS_INLINE void ezSimdTransform::SetIdentity()
{
  m_Position.SetZero();
  m_Rotation.SetIdentity();
  m_Scale.Set(1.0f);
}

//static
EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::Identity()
{
  ezSimdTransform result;
  result.SetIdentity();
  return result;
}

EZ_ALWAYS_INLINE bool ezSimdTransform::IsEqual(const ezSimdTransform& rhs, const ezSimdFloat& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() &&
    m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) &&
    m_Scale.IsEqual(rhs.m_Scale, fEpsilon).AllSet<3>();
}

EZ_ALWAYS_INLINE void ezSimdTransform::Invert()
{
  (*this) = GetInverse();
}

EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::GetInverse() const
{
  ezSimdQuat invRot = -m_Rotation;
  ezSimdVec4f invScale = m_Scale.GetReciprocal();
  ezSimdVec4f invPos = invRot * (invScale.CompMul(-m_Position));

  return ezSimdTransform(invPos, invRot, invScale);
}

inline void ezSimdTransform::SetLocalTransform(const ezSimdTransform& GlobalTransformParent, const ezSimdTransform& GlobalTransformChild)
{
  ezSimdQuat invRot = -GlobalTransformParent.m_Rotation;
  ezSimdVec4f invScale = GlobalTransformParent.m_Scale.GetReciprocal();

  m_Position = (invRot * (GlobalTransformChild.m_Position - GlobalTransformParent.m_Position)).CompMul(invScale);
  m_Rotation = invRot * GlobalTransformChild.m_Rotation;
  m_Scale = invScale.CompMul(GlobalTransformChild.m_Scale);
}

EZ_ALWAYS_INLINE void ezSimdTransform::SetGlobalTransform(const ezSimdTransform& GlobalTransformParent, const ezSimdTransform& LocalTransformChild)
{
  *this = GlobalTransformParent * LocalTransformChild;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdTransform::TransformPosition(const ezSimdVec4f& v) const
{
  const ezSimdVec4f scaled = m_Scale.CompMul(v);
  const ezSimdVec4f rotated = m_Rotation * scaled;
  return m_Position + rotated;
}

EZ_ALWAYS_INLINE ezSimdVec4f ezSimdTransform::TransformDirection(const ezSimdVec4f& v) const
{
  const ezSimdVec4f scaled = m_Scale.CompMul(v);
  return m_Rotation * scaled;
}

inline ezSimdTransform ezSimdTransform::operator*(const ezSimdTransform& other) const
{
  ezSimdTransform t;

  t.m_Position = (m_Rotation * other.m_Position.CompMul(m_Scale)) + m_Position;
  t.m_Rotation = m_Rotation * other.m_Rotation;
  t.m_Scale = m_Scale.CompMul(other.m_Scale);

  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransform::operator*=(const ezSimdTransform& other)
{
  (*this) = (*this) * other;
}

EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::operator*(const ezSimdQuat& q) const
{
  ezSimdTransform t;

  t.m_Position = m_Position;
  t.m_Rotation = m_Rotation * q;
  t.m_Scale = m_Scale;

  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransform::operator*=(const ezSimdQuat& q)
{
  m_Rotation = m_Rotation * q;
}

EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::operator+(const ezSimdVec4f& v) const
{
  ezSimdTransform t;

  t.m_Position = m_Position + v;
  t.m_Rotation = m_Rotation;
  t.m_Scale = m_Scale;

  return t;
}

EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::operator-(const ezSimdVec4f& v) const
{
  ezSimdTransform t;

  t.m_Position = m_Position - v;
  t.m_Rotation = m_Rotation;
  t.m_Scale = m_Scale;

  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransform::operator+=(const ezSimdVec4f& v)
{
  m_Position += v;
}

EZ_ALWAYS_INLINE void ezSimdTransform::operator-=(const ezSimdVec4f& v)
{
  m_Position -= v;
}

EZ_ALWAYS_INLINE bool ezSimdTransform::operator==(const ezSimdTransform& other) const
{
  return (m_Position == other.m_Position).AllSet<3>() && m_Rotation == other.m_Rotation && (m_Scale == other.m_Scale).AllSet<3>();
}

EZ_ALWAYS_INLINE bool ezSimdTransform::operator!=(const ezSimdTransform& other) const
{
  return !(*this == other);
}
