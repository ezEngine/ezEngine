#pragma once

#include <Foundation/Math/Transform.h>

template <typename Type>
inline ezTransformTemplate<Type>::ezTransformTemplate(const ezVec3Template<Type>& vPosition,
  const ezQuatTemplate<Type>& qRotation, const ezVec3Template<Type>& vScale)
  : m_vPosition(vPosition)
  , m_qRotation(qRotation)
  , m_vScale(vScale)
{
}

template <typename Type>
inline ezTransformTemplate<Type> ezTransformTemplate<Type>::Make(const ezVec3Template<Type>& vPosition, const ezQuatTemplate<Type>& qRotation /*= ezQuatTemplate<Type>::IdentityQuaternion()*/, const ezVec3Template<Type>& vScale /*= ezVec3Template<Type>(1)*/)
{
  ezTransformTemplate<Type> res;
  res.m_vPosition = vPosition;
  res.m_qRotation = qRotation;
  res.m_vScale = vScale;
  return res;
}

template <typename Type>
inline ezTransformTemplate<Type> ezTransformTemplate<Type>::MakeIdentity()
{
  ezTransformTemplate<Type> res;
  res.m_vPosition.SetZero();
  res.m_qRotation = ezQuatTemplate<Type>::MakeIdentity();
  res.m_vScale.Set(1.0f);
  return res;
}

template <typename Type>
ezTransformTemplate<Type> ezTransformTemplate<Type>::MakeFromMat4(const ezMat4Template<Type>& mMat)
{
  ezMat3Template<Type> mRot = mMat.GetRotationalPart();

  ezTransformTemplate<Type> res;
  res.m_vPosition = mMat.GetTranslationVector();
  res.m_vScale = mRot.GetScalingFactors();
  mRot.SetScalingFactors(ezVec3Template<Type>(1)).IgnoreResult();
  res.m_qRotation = ezQuat::MakeFromMat3(mRot);
  return res;
}

template <typename Type>
ezTransformTemplate<Type> ezTransformTemplate<Type>::MakeLocalTransform(const ezTransformTemplate& globalTransformParent, const ezTransformTemplate& globalTransformChild)
{
  const auto invRot = globalTransformParent.m_qRotation.GetInverse();
  const auto invScale = ezVec3Template<Type>(1).CompDiv(globalTransformParent.m_vScale);

  ezTransformTemplate<Type> res;
  res.m_vPosition = (invRot * (globalTransformChild.m_vPosition - globalTransformParent.m_vPosition)).CompMul(invScale);
  res.m_qRotation = invRot * globalTransformChild.m_qRotation;
  res.m_vScale = invScale.CompMul(globalTransformChild.m_vScale);
  return res;
}

template <typename Type>
EZ_ALWAYS_INLINE ezTransformTemplate<Type> ezTransformTemplate<Type>::MakeGlobalTransform(const ezTransformTemplate& globalTransformParent, const ezTransformTemplate& localTransformChild)
{
  return globalTransformParent * localTransformChild;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezTransformTemplate<Type>::SetIdentity()
{
  *this = MakeIdentity();
}

template <typename Type>
EZ_ALWAYS_INLINE Type ezTransformTemplate<Type>::GetMaxScale() const
{
  auto absScale = m_vScale.Abs();
  return ezMath::Max(absScale.x, ezMath::Max(absScale.y, absScale.z));
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezTransformTemplate<Type>::HasMirrorScaling() const
{
  return (m_vScale.x * m_vScale.y * m_vScale.z) < 0.0f;
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezTransformTemplate<Type>::ContainsUniformScale() const
{
  const Type fEpsilon = ezMath::DefaultEpsilon<Type>();
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
inline bool ezTransformTemplate<Type>::IsValid() const
{
  return m_vPosition.IsValid() && m_qRotation.IsValid(0.005f) && m_vScale.IsValid();
}

template <typename Type>
EZ_ALWAYS_INLINE const ezMat4Template<Type> ezTransformTemplate<Type>::GetAsMat4() const
{
  auto result = m_qRotation.GetAsMat4();

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
EZ_ALWAYS_INLINE void ezTransformTemplate<Type>::operator+=(const ezVec3Template<Type>& v)
{
  m_vPosition += v;
}

template <typename Type>
EZ_ALWAYS_INLINE void ezTransformTemplate<Type>::operator-=(const ezVec3Template<Type>& v)
{
  m_vPosition -= v;
}

template <typename Type>
EZ_ALWAYS_INLINE ezVec3Template<Type> ezTransformTemplate<Type>::TransformPosition(const ezVec3Template<Type>& v) const
{
  const auto scaled = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
  return m_vPosition + rotated;
}

template <typename Type>
EZ_ALWAYS_INLINE ezVec3Template<Type> ezTransformTemplate<Type>::TransformDirection(const ezVec3Template<Type>& v) const
{
  const auto scaled = m_vScale.CompMul(v);
  const auto rotated = m_qRotation * scaled;
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
  const auto invRot = m_qRotation.GetInverse();
  const auto invScale = ezVec3Template<Type>(1).CompDiv(m_vScale);
  const auto invPos = invRot * (invScale.CompMul(-m_vPosition));

  return ezTransformTemplate<Type>(invPos, invRot, invScale);
}
