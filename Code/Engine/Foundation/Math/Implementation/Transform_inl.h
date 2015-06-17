#pragma once

#include <Foundation/Math/Transform.h>

template<typename Type>
inline ezTransformTemplate<Type>::ezTransformTemplate(const ezVec3Template<Type>& vPosition, const ezMat3Template<Type>& Rotation)
{
  m_vPosition = vPosition;
  m_Rotation = Rotation;
}

template<typename Type>
inline ezTransformTemplate<Type>::ezTransformTemplate(const ezVec3Template<Type>& vPosition, const ezQuatTemplate<Type>& qRotation)
{
  m_vPosition = vPosition;
  m_Rotation = qRotation.GetAsMat3();
}

template<typename Type>
inline ezTransformTemplate<Type>::ezTransformTemplate(const ezVec3Template<Type>& vPosition, const ezQuatTemplate<Type>& qRotation, const ezVec3Template<Type>& vScale)
{
  m_vPosition = vPosition;
  m_Rotation = qRotation.GetAsMat3();

  if (!vScale.IsIdentical(ezVec3Template<Type>(1)))
    m_Rotation.SetScalingFactors(vScale);
}

template<typename Type>
inline void ezTransformTemplate<Type>::SetIdentity()
{
  m_vPosition.SetZero();
  m_Rotation.SetIdentity();
}

template<typename Type>
inline bool ezTransformTemplate<Type>::IsIdentical(const ezTransformTemplate<Type>& rhs) const
{
  return m_vPosition.IsIdentical(rhs.m_vPosition) && m_Rotation.IsIdentical(rhs.m_Rotation);
}

template<typename Type>
inline bool ezTransformTemplate<Type>::IsEqual(const ezTransformTemplate<Type>& rhs, Type fEpsilon) const
{
  return m_vPosition.IsEqual(rhs.m_vPosition, fEpsilon) && m_Rotation.IsEqual(rhs.m_Rotation, fEpsilon);
}

template<typename Type>
inline void ezTransformTemplate<Type>::SetLocalTransform(const ezTransformTemplate<Type>& GlobalTransformParent, const ezTransformTemplate<Type>& GlobalTransformChild)
{
  const ezMat3Template<Type> mParentInverseRot = GlobalTransformParent.m_Rotation.GetInverse();

  m_vPosition = mParentInverseRot * (GlobalTransformChild.m_vPosition - GlobalTransformParent.m_vPosition);
  m_Rotation  = mParentInverseRot * GlobalTransformChild.m_Rotation;
}

template<typename Type>
inline void ezTransformTemplate<Type>::SetGlobalTransform(const ezTransformTemplate<Type>& GlobalTransformParent, const ezTransformTemplate<Type>& LocalTransformChild)
{
  m_vPosition = GlobalTransformParent.m_vPosition + (GlobalTransformParent.m_Rotation * LocalTransformChild.m_vPosition);
  m_Rotation = GlobalTransformParent.m_Rotation * LocalTransformChild.m_Rotation;
}

template<typename Type>
inline void ezTransformTemplate<Type>::Decompose(ezVec3& vPos, ezQuat& qRot, ezVec3& vScale) const
{
  /// \test This is new

  vPos = m_vPosition;
  vScale = m_Rotation.GetScalingFactors();

  ezMat3 mRot = m_Rotation;
  mRot.SetScalingFactors(ezVec3(1.0f)); /// \todo Can we make this more efficient? E.g. ezQuat::SetFromScaledMat3 ?
  qRot.SetFromMat3(mRot);
}

template<typename Type>
EZ_FORCE_INLINE const ezMat4Template<Type> ezTransformTemplate<Type>::GetAsMat4() const
{
  return ezMat4Template<Type>(m_Rotation, m_vPosition);
}

template<typename Type>
EZ_FORCE_INLINE void ezTransformTemplate<Type>::operator*=(const ezQuatTemplate<Type>& q)
{
  m_Rotation = q.GetAsMat3() * m_Rotation;
}

template<typename Type>
EZ_FORCE_INLINE const ezTransformTemplate<Type> operator*(const ezQuatTemplate<Type>& q, const ezTransformTemplate<Type>& t)
{
  return ezTransformTemplate<Type>(t.m_vPosition, q.GetAsMat3() * t.m_Rotation);
}

template<typename Type>
EZ_FORCE_INLINE const ezTransformTemplate<Type> operator+(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return ezTransformTemplate<Type>(t.m_vPosition + v, t.m_Rotation);
}

template<typename Type>
EZ_FORCE_INLINE const ezTransformTemplate<Type> operator-(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return ezTransformTemplate<Type>(t.m_vPosition - v, t.m_Rotation);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> operator*(const ezTransformTemplate<Type>& t, const ezVec3Template<Type>& v)
{
  return (t.m_Rotation * v) + t.m_vPosition;
}

template<typename Type>
inline const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t1, const ezTransformTemplate<Type>& t2)
{
  ezTransformTemplate<Type> r;
  r.m_Rotation = t1.m_Rotation * t2.m_Rotation;
  r.m_vPosition = t1.m_Rotation * t2.m_vPosition + t1.m_vPosition;

  return r;
}

template<typename Type>
inline const ezTransformTemplate<Type> operator*(const ezTransformTemplate<Type>& t1, const ezMat4& t2)
{
  ezTransformTemplate<Type> r;
  r.m_Rotation = t1.m_Rotation * t2.GetRotationalPart();
  r.m_vPosition = t1.m_Rotation * t2.GetTranslationVector() + t1.m_vPosition;

  return r;
}

template<typename Type>
inline const ezTransformTemplate<Type> operator*(const ezMat4& t1, const ezTransformTemplate<Type>& t2)
{
  ezTransformTemplate<Type> r;
  r.m_Rotation = t1.GetRotationalPart() * t2.m_Rotation;
  r.m_vPosition = t1.GetRotationalPart() * t2.m_vPosition + t1.GetTranslationVector();

  return r;
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
inline ezResult ezTransformTemplate<Type>::Invert(Type fEpsilon)
{
  if (m_Rotation.Invert(fEpsilon).Failed())
    return EZ_FAILURE;

  m_vPosition = m_Rotation * -m_vPosition;

  return EZ_SUCCESS;
}

template<typename Type>
inline const ezTransformTemplate<Type> ezTransformTemplate<Type>::GetInverse() const
{
  ezTransformTemplate<Type> Inverse = *this;
  Inverse.Invert();
  return Inverse;
}

template<typename Type>
inline void ezTransformTemplate<Type>::GetAsArray(Type* out_pData, ezMatrixLayout::Enum layout) const
{
  if (layout == ezMatrixLayout::ColumnMajor)
  {
    ezMemoryUtils::Copy(out_pData, &m_Rotation.m_fElementsCM[0], 12);
  }
  else
  {
    out_pData[0] = m_Rotation.Element(0, 0);
    out_pData[1] = m_Rotation.Element(1, 0);
    out_pData[2] = m_Rotation.Element(2, 0);
    out_pData[3] = m_vPosition.x;

    out_pData[4] = m_Rotation.Element(0, 1);
    out_pData[5] = m_Rotation.Element(1, 1);
    out_pData[6] = m_Rotation.Element(2, 1);
    out_pData[7] = m_vPosition.y;

    out_pData[8] = m_Rotation.Element(0, 2);
    out_pData[9] = m_Rotation.Element(1, 2);
    out_pData[10]= m_Rotation.Element(2, 2);
    out_pData[11]= m_vPosition.z;
  }
}
