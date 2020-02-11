#pragma once

EZ_ALWAYS_INLINE ezSimdTransform::ezSimdTransform() {}

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

// static
EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::IdentityTransform()
{
  ezSimdTransform result;
  result.SetIdentity();
  return result;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdTransform::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

EZ_ALWAYS_INLINE bool ezSimdTransform::ContainsNegativeScale() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < ezSimdFloat::Zero();
}

EZ_ALWAYS_INLINE bool ezSimdTransform::ContainsUniformScale() const
{
  const ezSimdFloat fEpsilon = ezMath::DefaultEpsilon<float>();
  return m_Scale.x().IsEqual(m_Scale.y(), fEpsilon) && m_Scale.x().IsEqual(m_Scale.z(), fEpsilon);
}

EZ_ALWAYS_INLINE bool ezSimdTransform::IsEqual(const ezSimdTransform& rhs, const ezSimdFloat& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() && m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) &&
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

EZ_ALWAYS_INLINE void ezSimdTransform::SetGlobalTransform(const ezSimdTransform& GlobalTransformParent,
                                                          const ezSimdTransform& LocalTransformChild)
{
  *this = GlobalTransformParent * LocalTransformChild;
}

EZ_FORCE_INLINE ezSimdMat4f ezSimdTransform::GetAsMat4() const
{
  ezSimdMat4f result = m_Rotation.GetAsMat4();

  result.m_col0 *= m_Scale.x();
  result.m_col1 *= m_Scale.y();
  result.m_col2 *= m_Scale.z();
  result.m_col3 = m_Position;
  result.m_col3.SetW(1.0f);

  return result;
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

EZ_ALWAYS_INLINE const ezSimdVec4f operator*(const ezSimdTransform& t, const ezSimdVec4f& v)
{
  return t.TransformPosition(v);
}

inline const ezSimdTransform operator*(const ezSimdTransform& lhs, const ezSimdTransform& rhs)
{
  ezSimdTransform t;

  t.m_Position = (lhs.m_Rotation * rhs.m_Position.CompMul(lhs.m_Scale)) + lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * rhs.m_Rotation;
  t.m_Scale = lhs.m_Scale.CompMul(rhs.m_Scale);

  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransform::operator*=(const ezSimdTransform& other)
{
  (*this) = (*this) * other;
}

EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdTransform& lhs, const ezSimdQuat& q)
{
  ezSimdTransform t;
  t.m_Position = lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * q;
  t.m_Scale = lhs.m_Scale;
  return t;
}

EZ_ALWAYS_INLINE const ezSimdTransform operator*(const ezSimdQuat& q, const ezSimdTransform& rhs)
{
  ezSimdTransform t;
  t.m_Position = rhs.m_Position;
  t.m_Rotation = q * rhs.m_Rotation;
  t.m_Scale = rhs.m_Scale;
  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransform::operator*=(const ezSimdQuat& q)
{
  m_Rotation = m_Rotation * q;
}

EZ_ALWAYS_INLINE const ezSimdTransform operator+(const ezSimdTransform& lhs, const ezSimdVec4f& v)
{
  ezSimdTransform t;

  t.m_Position = lhs.m_Position + v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

EZ_ALWAYS_INLINE const ezSimdTransform operator-(const ezSimdTransform& lhs, const ezSimdVec4f& v)
{
  ezSimdTransform t;

  t.m_Position = lhs.m_Position - v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

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

EZ_ALWAYS_INLINE bool operator==(const ezSimdTransform& lhs, const ezSimdTransform& rhs)
{
  return (lhs.m_Position == rhs.m_Position).AllSet<3>() && lhs.m_Rotation == rhs.m_Rotation && (lhs.m_Scale == rhs.m_Scale).AllSet<3>();
}

EZ_ALWAYS_INLINE bool operator!=(const ezSimdTransform& lhs, const ezSimdTransform& rhs)
{
  return !(lhs == rhs);
}

