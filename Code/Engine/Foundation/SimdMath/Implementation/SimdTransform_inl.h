#pragma once

EZ_ALWAYS_INLINE ezSimdTransform::ezSimdTransform() = default;

EZ_ALWAYS_INLINE ezSimdTransform::ezSimdTransform(const ezSimdVec4f& vPosition, const ezSimdQuat& qRotation, const ezSimdVec4f& vScale)
  : m_Position(vPosition)
  , m_Rotation(qRotation)
  , m_Scale(vScale)
{
}

EZ_ALWAYS_INLINE ezSimdTransform::ezSimdTransform(const ezSimdQuat& qRotation)
  : m_Rotation(qRotation)
{
  m_Position.SetZero();
  m_Scale.Set(1.0f);
}

inline ezSimdTransform ezSimdTransform::Make(const ezSimdVec4f& vPosition, const ezSimdQuat& qRotation /*= ezSimdQuat::IdentityQuaternion()*/, const ezSimdVec4f& vScale /*= ezSimdVec4f(1.0f)*/)
{
  ezSimdTransform res;
  res.m_Position = vPosition;
  res.m_Rotation = qRotation;
  res.m_Scale = vScale;
  return res;
}

EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::MakeIdentity()
{
  ezSimdTransform res;
  res.m_Position.SetZero();
  res.m_Rotation = ezSimdQuat::MakeIdentity();
  res.m_Scale.Set(1.0f);
  return res;
}

inline ezSimdTransform ezSimdTransform::MakeLocalTransform(const ezSimdTransform& globalTransformParent, const ezSimdTransform& globalTransformChild)
{
  const ezSimdQuat invRot = -globalTransformParent.m_Rotation;
  const ezSimdVec4f invScale = globalTransformParent.m_Scale.GetReciprocal();

  ezSimdTransform res;
  res.m_Position = (invRot * (globalTransformChild.m_Position - globalTransformParent.m_Position)).CompMul(invScale);
  res.m_Rotation = invRot * globalTransformChild.m_Rotation;
  res.m_Scale = invScale.CompMul(globalTransformChild.m_Scale);
  return res;
}

EZ_ALWAYS_INLINE ezSimdTransform ezSimdTransform::MakeGlobalTransform(const ezSimdTransform& globalTransformParent, const ezSimdTransform& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

EZ_ALWAYS_INLINE ezSimdFloat ezSimdTransform::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

EZ_ALWAYS_INLINE bool ezSimdTransform::HasMirrorScaling() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < ezSimdFloat::MakeZero();
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
