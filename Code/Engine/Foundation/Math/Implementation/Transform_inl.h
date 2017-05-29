#pragma once

#include <Foundation/Math/Transform.h>

template<typename Type>
inline ezTransformTemplate<Type>::ezTransformTemplate(const ezVec3Template<Type>& vPosition, const ezQuatTemplate<Type>& qRotation)
{
  m_vPosition = vPosition;
  m_qRotation = qRotation;
  m_vScale.Set(1.0f);
}

template<typename Type>
inline ezTransformTemplate<Type>::ezTransformTemplate(const ezVec3Template<Type>& vPosition, const ezQuatTemplate<Type>& qRotation, const ezVec3Template<Type>& vScale)
{
  m_vPosition = vPosition;
  m_qRotation = qRotation;
  m_vScale = vScale;
}


template<typename Type>
void ezTransformTemplate<Type>::SetFromMat4(const ezMat4& mat)
{
  ezMat3 mRot = mat.GetRotationalPart();

  m_vPosition = mat.GetTranslationVector();
  m_vScale = mRot.GetScalingFactors();
  mRot.SetScalingFactors(ezVec3(1.0f));
  m_qRotation.SetFromMat3(mRot);
}

template<typename Type>
inline void ezTransformTemplate<Type>::SetIdentity()
{
  m_vPosition.SetZero();
  m_qRotation.SetIdentity();
  m_vScale.Set(1);
}

//static
template<typename Type>
inline const ezTransformTemplate<Type> ezTransformTemplate<Type>::Identity()
{
  return ezTransformTemplate<Type>(ezVec3Template<Type>::ZeroVector(), ezQuatTemplate<Type>::IdentityQuaternion(), ezVec3Template<Type>(1));
}

template<typename Type>
inline bool ezTransformTemplate<Type>::IsIdentical(const ezTransformTemplate<Type>& rhs) const
{
  return m_vPosition.IsIdentical(rhs.m_vPosition) && (m_qRotation == rhs.m_qRotation) && m_vScale.IsIdentical(rhs.m_vScale);
}

template<typename Type>
inline bool ezTransformTemplate<Type>::IsEqual(const ezTransformTemplate<Type>& rhs, Type fEpsilon) const
{
  return m_vPosition.IsEqual(rhs.m_vPosition, fEpsilon) && m_qRotation.IsEqualRotation(rhs.m_qRotation, fEpsilon) && m_vScale.IsEqual(rhs.m_vScale, fEpsilon);
}

template<typename Type>
inline void ezTransformTemplate<Type>::SetLocalTransform(const ezTransformTemplate<Type>& GlobalTransformParent, const ezTransformTemplate<Type>& GlobalTransformChild)
{
  const ezQuat invRot = -GlobalTransformParent.m_qRotation;
  const ezVec3 invScale = ezVec3(1.0f).CompDiv(GlobalTransformParent.m_vScale);

  m_vPosition = (invRot * (GlobalTransformChild.m_vPosition - GlobalTransformParent.m_vPosition)).CompMul(invScale);
  m_qRotation = invRot * GlobalTransformChild.m_qRotation;
  m_vScale = invScale.CompMul(GlobalTransformChild.m_vScale);
}

template<typename Type>
inline void ezTransformTemplate<Type>::SetGlobalTransform(const ezTransformTemplate<Type>& GlobalTransformParent, const ezTransformTemplate<Type>& LocalTransformChild)
{
  *this = GlobalTransformParent * LocalTransformChild;
}

template<typename Type>
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

template<typename Type>
EZ_ALWAYS_INLINE void ezTransformTemplate<Type>::operator*=(const ezQuatTemplate<Type>& q)
{
  m_qRotation = q * m_qRotation;
}

template<typename Type>
EZ_ALWAYS_INLINE const ezTransformTemplate<Type> operator*(const ezQuatTemplate<Type>& q, const ezTransformTemplate<Type>& t)
{
  ezTransform r;

  r.m_vPosition = t.m_vPosition;
  r.m_qRotation = t.m_qRotation * q;
  r.m_vScale = t.m_vScale;

  return r;
}

template<typename Type>
EZ_ALWAYS_INLINE const ezTransformTemplate<Type> operator+(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return ezTransformTemplate<Type>(t.m_vPosition + v, t.m_qRotation, t.m_vScale);
}

template<typename Type>
EZ_ALWAYS_INLINE const ezTransformTemplate<Type> operator-(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return ezTransformTemplate<Type>(t.m_vPosition - v, t.m_qRotation, t.m_vScale);
}

template<typename Type>
EZ_ALWAYS_INLINE const ezVec3Template<Type> operator*(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  const ezVec3 scaled = t.m_vScale.CompMul(v);
  const ezVec3 rotated = t.m_qRotation * scaled;
  return t.m_vPosition + rotated;
}

template<typename Type>
inline const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2)
{
  ezTransformTemplate<Type> t;

  t.m_vPosition = (t1.m_qRotation * t2.m_vPosition.CompMul(t1.m_vScale)) + t1.m_vPosition;
  t.m_qRotation = t1.m_qRotation * t2.m_qRotation;
  t.m_vScale = t1.m_vScale.CompMul(t2.m_vScale);

  return t;
}

template<typename Type>
inline bool operator==(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2)
{
  return t1.IsIdentical(t2);
}

template<typename Type>
inline bool operator!=(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2)
{
  return !t1.IsIdentical(t2);
}


template<typename Type>
inline void ezTransformTemplate<Type>::Invert()
{
  (*this) = GetInverse();
}

template<typename Type>
inline const ezTransformTemplate<Type> ezTransformTemplate<Type>::GetInverse() const
{
  const ezQuat invRot = -m_qRotation;
  const ezVec3 invScale = ezVec3(1.0f).CompDiv(m_vScale);
  const ezVec3 invPos = invRot * (invScale.CompMul(-m_vPosition));

  return ezTransformTemplate<Type>(invPos, invRot, invScale);
}

//template<typename Type>
//inline void ezTransformTemplate<Type>::GetAsArray(Type* out_pData, ezMatrixLayout::Enum layout) const
//{
//  if (layout == ezMatrixLayout::ColumnMajor)
//  {
//    ezMemoryUtils::Copy(out_pData, &m_Rotation.m_fElementsCM[0], 12);
//  }
//  else
//  {
//    out_pData[0] = m_Rotation.Element(0, 0);
//    out_pData[1] = m_Rotation.Element(1, 0);
//    out_pData[2] = m_Rotation.Element(2, 0);
//    out_pData[3] = m_vPosition.x;
//
//    out_pData[4] = m_Rotation.Element(0, 1);
//    out_pData[5] = m_Rotation.Element(1, 1);
//    out_pData[6] = m_Rotation.Element(2, 1);
//    out_pData[7] = m_vPosition.y;
//
//    out_pData[8] = m_Rotation.Element(0, 2);
//    out_pData[9] = m_Rotation.Element(1, 2);
//    out_pData[10] = m_Rotation.Element(2, 2);
//    out_pData[11] = m_vPosition.z;
//  }
//}

