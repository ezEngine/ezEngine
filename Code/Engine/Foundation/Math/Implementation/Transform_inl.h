#pragma once

#include <Foundation/Math/Transform.h>

template <typename Type>
inline ezTransformTemplate<Type>::ezTransformTemplate(const ezVec3Template<Type>& vPosition, const ezQuatTemplate<Type>& qRotation, const ezVec3Template<Type>& vScale)
{
  m_vPosition = vPosition;
  m_qRotation = qRotation;
  m_vScale = vScale;
}

template <typename Type>
void ezTransformTemplate<Type>::SetFromMat4(const ezMat4& mat)
{
  ezMat3 mRot = mat.GetRotationalPart();

  m_vPosition = mat.GetTranslationVector();
  m_vScale = mRot.GetScalingFactors();
  mRot.SetScalingFactors(ezVec3(1.0f));
  m_qRotation.SetFromMat3(mRot);
}

template <typename Type>
inline void ezTransformTemplate<Type>::SetIdentity()
{
  m_vPosition.SetZero();
  m_qRotation.SetIdentity();
  m_vScale.Set(1);
}

//static
template <typename Type>
inline const ezTransformTemplate<Type> ezTransformTemplate<Type>::IdentityTransform()
{
  return ezTransformTemplate<Type>(ezVec3Template<Type>::ZeroVector(), ezQuatTemplate<Type>::IdentityQuaternion(), ezVec3Template<Type>(1));
}

template <typename Type>
EZ_ALWAYS_INLINE Type ezTransformTemplate<Type>::GetMaxScale() const
{
  ezVec3 absScale = m_vScale.Abs();
  return ezMath::Max(absScale.x, ezMath::Max(absScale.y, absScale.z));
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezTransformTemplate<Type>::ContainsNegativeScale() const
{
  return (m_vScale.x * m_vScale.y * m_vScale.z) < 0.0f;
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezTransformTemplate<Type>::ContainsUniformScale() const
{
  const Type fEpsilon = ezMath::BasicType<Type>::DefaultEpsilon();
  return ezMath::IsEqual(m_vScale.x, m_vScale.y, fEpsilon) && ezMath::IsEqual(m_vScale.x, m_vScale.z, fEpsilon);
}

template <typename Type>
inline bool ezTransformTemplate<Type>::IsIdentical(const ezTransformTemplate<Type>& rhs) const
{
  return m_vPosition.IsIdentical(rhs.m_vPosition) && (m_qRotation == rhs.m_qRotation) && m_vScale.IsIdentical(rhs.m_vScale);
}

template <typename Type>
inline bool ezTransformTemplate<Type>::IsEqual(const ezTransformTemplate<Type>& rhs, Type fEpsilon) const
{
  return m_vPosition.IsEqual(rhs.m_vPosition, fEpsilon) && m_qRotation.IsEqualRotation(rhs.m_qRotation, fEpsilon) && m_vScale.IsEqual(rhs.m_vScale, fEpsilon);
}

template <typename Type>
inline void ezTransformTemplate<Type>::SetLocalTransform(const ezTransformTemplate<Type>& GlobalTransformParent, const ezTransformTemplate<Type>& GlobalTransformChild)
{
  const ezQuat invRot = -GlobalTransformParent.m_qRotation;
  const ezVec3 invScale = ezVec3(1.0f).CompDiv(GlobalTransformParent.m_vScale);

  m_vPosition = (invRot * (GlobalTransformChild.m_vPosition - GlobalTransformParent.m_vPosition)).CompMul(invScale);
  m_qRotation = invRot * GlobalTransformChild.m_qRotation;
  m_vScale = invScale.CompMul(GlobalTransformChild.m_vScale);
}

template <typename Type>
inline void ezTransformTemplate<Type>::SetGlobalTransform(const ezTransformTemplate<Type>& GlobalTransformParent, const ezTransformTemplate<Type>& LocalTransformChild)
{
  *this = GlobalTransformParent * LocalTransformChild;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezMat4Template<Type> ezTransformTemplate<Type>::GetAsMat4() const
{
  ezMat4 result = m_qRotation.GetAsMat4();

  result.m_fElementsCM[0] *= m_vScale.x;
  result.m_fElementsCM[1] *= m_vScale.x;
  result.m_fElementsCM[2] *= m_vScale.x;

  result.m_fElementsCM[4] *= m_vScale.y;
  result.m_fElementsCM[5] *= m_vScale.y;
  result.m_fElementsCM[6] *= m_vScale.y;

  result.m_fElementsCM[8] *= m_vScale.z;
  result.m_fElementsCM[9] *= m_vScale.z;
  result.m_fElementsCM[10] *= m_vScale.z;

  result.m_fElementsCM[12] = m_vPosition.x;
  result.m_fElementsCM[13] = m_vPosition.y;
  result.m_fElementsCM[14] = m_vPosition.z;

  return result;
}


template <typename Type>
void ezTransformTemplate<Type>::operator+=(const ezVec3& v)
{
  m_vPosition += v;
}

template <typename Type>
void ezTransformTemplate<Type>::operator-=(const ezVec3& v)
{
  m_vPosition -= v;
}

template <typename Type>
EZ_ALWAYS_INLINE ezVec3 ezTransformTemplate<Type>::TransformPosition(const ezVec3& v) const
{
  const ezVec3 scaled = m_vScale.CompMul(v);
  const ezVec3 rotated = m_qRotation * scaled;
  return m_vPosition + rotated;
}

template <typename Type>
EZ_ALWAYS_INLINE ezVec3 ezTransformTemplate<Type>::TransformDirection(const ezVec3& v) const
{
  const ezVec3 scaled = m_vScale.CompMul(v);
  const ezVec3 rotated = m_qRotation * scaled;
  return rotated;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezTransformTemplate<Type> operator*(const ezQuatTemplate<Type>& q, const ezTransformTemplate<Type>& t)
{
  ezTransform r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = q * t.m_qRotation;
  r.m_vScale = t.m_vScale;

  return r;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t, const ezQuatTemplate<Type>& q)
{
  ezTransform r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = t.m_qRotation * q;
  r.m_vScale = t.m_vScale;

  return r;
}

template <typename Type>
EZ_ALWAYS_INLINE const ezTransformTemplate<Type> operator+(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return ezTransformTemplate<Type>(t.m_vPosition + v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
EZ_ALWAYS_INLINE const ezTransformTemplate<Type> operator-(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return ezTransformTemplate<Type>(t.m_vPosition - v, t.m_qRotation, t.m_vScale);
}

template <typename Type>
EZ_ALWAYS_INLINE const ezVec3Template<Type> operator*(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return t.TransformPosition(v);
}

template <typename Type>
inline const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2)
{
  ezTransformTemplate<Type> t;

  t.m_vPosition = (t1.m_qRotation * t2.m_vPosition.CompMul(t1.m_vScale)) + t1.m_vPosition;
  t.m_qRotation = t1.m_qRotation * t2.m_qRotation;
  t.m_vScale = t1.m_vScale.CompMul(t2.m_vScale);

  return t;
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator==(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2)
{
  return t1.IsIdentical(t2);
}

template <typename Type>
EZ_ALWAYS_INLINE bool operator!=(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2)
{
  return !t1.IsIdentical(t2);
}

template <typename Type>
EZ_ALWAYS_INLINE void ezTransformTemplate<Type>::Invert()
{
  (*this) = GetInverse();
}

template <typename Type>
inline const ezTransformTemplate<Type> ezTransformTemplate<Type>::GetInverse() const
{
  const ezQuat invRot = -m_qRotation;
  const ezVec3 invScale = ezVec3(1.0f).CompDiv(m_vScale);
  const ezVec3 invPos = invRot * (invScale.CompMul(-m_vPosition));

  return ezTransformTemplate<Type>(invPos, invRot, invScale);
}

