#pragma once

EZ_ALWAYS_INLINE ezSimdTransformd::ezSimdTransformd() = default;

EZ_ALWAYS_INLINE ezSimdTransformd::ezSimdTransformd(const ezSimdVec4d& vPosition, const ezSimdQuatd& qRotation, const ezSimdVec4d& vScale)
  : m_Position(vPosition)
  , m_Rotation(qRotation)
  , m_Scale(vScale)
{
}

EZ_ALWAYS_INLINE ezSimdTransformd::ezSimdTransformd(const ezSimdQuatd& qRotation)
  : m_Rotation(qRotation)
{
  m_Position.SetZero();
  m_Scale.Set(1.0f);
}

inline ezSimdTransformd ezSimdTransformd::Make(const ezSimdVec4d& vPosition, const ezSimdQuatd& qRotation /*= ezSimdQuatd::IdentityQuaternion()*/, const ezSimdVec4d& vScale /*= ezSimdVec4d(1.0f)*/)
{
  ezSimdTransformd res;
  res.m_Position = vPosition;
  res.m_Rotation = qRotation;
  res.m_Scale = vScale;
  return res;
}

EZ_ALWAYS_INLINE ezSimdTransformd ezSimdTransformd::MakeIdentity()
{
  ezSimdTransformd res;
  res.m_Position.SetZero();
  res.m_Rotation = ezSimdQuatd::MakeIdentity();
  res.m_Scale.Set(1.0f);
  return res;
}

inline ezSimdTransformd ezSimdTransformd::MakeLocalTransform(const ezSimdTransformd& globalTransformParent, const ezSimdTransformd& globalTransformChild)
{
  const ezSimdQuatd invRot = -globalTransformParent.m_Rotation;
  const ezSimdVec4d invScale = globalTransformParent.m_Scale.GetReciprocal();

  ezSimdTransformd res;
  res.m_Position = (invRot * (globalTransformChild.m_Position - globalTransformParent.m_Position)).CompMul(invScale);
  res.m_Rotation = invRot * globalTransformChild.m_Rotation;
  res.m_Scale = invScale.CompMul(globalTransformChild.m_Scale);
  return res;
}

EZ_ALWAYS_INLINE ezSimdTransformd ezSimdTransformd::MakeGlobalTransform(const ezSimdTransformd& globalTransformParent, const ezSimdTransformd& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

EZ_ALWAYS_INLINE ezSimdDouble ezSimdTransformd::GetMaxScale() const
{
  return m_Scale.Abs().HorizontalMax<3>();
}

EZ_ALWAYS_INLINE bool ezSimdTransformd::HasMirrorScaling() const
{
  return (m_Scale.x() * m_Scale.y() * m_Scale.z()) < ezSimdDouble::MakeZero();
}

EZ_ALWAYS_INLINE bool ezSimdTransformd::HasOnlyUniformScaling() const
{
  const ezSimdDouble fEpsilon = ezMath::DefaultEpsilon<float>();
  return m_Scale.x().IsEqual(m_Scale.y(), fEpsilon) && m_Scale.x().IsEqual(m_Scale.z(), fEpsilon);
}

EZ_ALWAYS_INLINE bool ezSimdTransformd::IsEqual(const ezSimdTransformd& rhs, const ezSimdDouble& fEpsilon) const
{
  return m_Position.IsEqual(rhs.m_Position, fEpsilon).AllSet<3>() && m_Rotation.IsEqualRotation(rhs.m_Rotation, fEpsilon) &&
         m_Scale.IsEqual(rhs.m_Scale, fEpsilon).AllSet<3>();
}

EZ_ALWAYS_INLINE void ezSimdTransformd::Invert()
{
  (*this) = GetInverse();
}

EZ_ALWAYS_INLINE ezSimdTransformd ezSimdTransformd::GetInverse() const
{
  ezSimdQuatd invRot = -m_Rotation;
  ezSimdVec4d invScale = m_Scale.GetReciprocal();
  ezSimdVec4d invPos = invRot * (invScale.CompMul(-m_Position));

  return ezSimdTransformd(invPos, invRot, invScale);
}

EZ_FORCE_INLINE ezSimdMat4d ezSimdTransformd::GetAsMat4() const
{
  ezSimdMat4d result = m_Rotation.GetAsMat4();

  result.m_col0 *= m_Scale.x();
  result.m_col1 *= m_Scale.y();
  result.m_col2 *= m_Scale.z();
  result.m_col3 = m_Position;
  result.m_col3.SetW(1.0f);

  return result;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdTransformd::TransformPosition(const ezSimdVec4d& v) const
{
  const ezSimdVec4d scaled = m_Scale.CompMul(v);
  const ezSimdVec4d rotated = m_Rotation * scaled;
  return m_Position + rotated;
}

EZ_ALWAYS_INLINE ezSimdVec4d ezSimdTransformd::TransformDirection(const ezSimdVec4d& v) const
{
  const ezSimdVec4d scaled = m_Scale.CompMul(v);
  return m_Rotation * scaled;
}

EZ_ALWAYS_INLINE const ezSimdVec4d operator*(const ezSimdTransformd& t, const ezSimdVec4d& v)
{
  return t.TransformPosition(v);
}

inline const ezSimdTransformd operator*(const ezSimdTransformd& lhs, const ezSimdTransformd& rhs)
{
  ezSimdTransformd t;

  t.m_Position = (lhs.m_Rotation * rhs.m_Position.CompMul(lhs.m_Scale)) + lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * rhs.m_Rotation;
  t.m_Scale = lhs.m_Scale.CompMul(rhs.m_Scale);

  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransformd::operator*=(const ezSimdTransformd& other)
{
  (*this) = (*this) * other;
}

EZ_ALWAYS_INLINE const ezSimdTransformd operator*(const ezSimdTransformd& lhs, const ezSimdQuatd& q)
{
  ezSimdTransformd t;
  t.m_Position = lhs.m_Position;
  t.m_Rotation = lhs.m_Rotation * q;
  t.m_Scale = lhs.m_Scale;
  return t;
}

EZ_ALWAYS_INLINE const ezSimdTransformd operator*(const ezSimdQuatd& q, const ezSimdTransformd& rhs)
{
  ezSimdTransformd t;
  t.m_Position = rhs.m_Position;
  t.m_Rotation = q * rhs.m_Rotation;
  t.m_Scale = rhs.m_Scale;
  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransformd::operator*=(const ezSimdQuatd& q)
{
  m_Rotation = m_Rotation * q;
}

EZ_ALWAYS_INLINE const ezSimdTransformd operator+(const ezSimdTransformd& lhs, const ezSimdVec4d& v)
{
  ezSimdTransformd t;

  t.m_Position = lhs.m_Position + v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

EZ_ALWAYS_INLINE const ezSimdTransformd operator-(const ezSimdTransformd& lhs, const ezSimdVec4d& v)
{
  ezSimdTransformd t;

  t.m_Position = lhs.m_Position - v;
  t.m_Rotation = lhs.m_Rotation;
  t.m_Scale = lhs.m_Scale;

  return t;
}

EZ_ALWAYS_INLINE void ezSimdTransformd::operator+=(const ezSimdVec4d& v)
{
  m_Position += v;
}

EZ_ALWAYS_INLINE void ezSimdTransformd::operator-=(const ezSimdVec4d& v)
{
  m_Position -= v;
}

EZ_ALWAYS_INLINE bool operator==(const ezSimdTransformd& lhs, const ezSimdTransformd& rhs)
{
  return (lhs.m_Position == rhs.m_Position).AllSet<3>() && lhs.m_Rotation == rhs.m_Rotation && (lhs.m_Scale == rhs.m_Scale).AllSet<3>();
}

EZ_ALWAYS_INLINE bool operator!=(const ezSimdTransformd& lhs, const ezSimdTransformd& rhs)
{
  return !(lhs == rhs);
}
