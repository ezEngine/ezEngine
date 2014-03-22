#pragma once

#include <Foundation/Math/Transform.h>

inline ezTransform::ezTransform(const ezVec3& vPosition, const ezMat3& Rotation)
{
  m_vPosition = vPosition;
  m_Rotation = Rotation;
}

inline ezTransform::ezTransform(const ezVec3& vPosition, const ezQuat& qRotation)
{
  m_vPosition = vPosition;
  m_Rotation = qRotation.GetAsMat3();
}

inline ezTransform::ezTransform(const ezVec3& vPosition, const ezQuat& qRotation, const ezVec3& vScale)
{
  m_vPosition = vPosition;
  m_Rotation = qRotation.GetAsMat3();

  if (!vScale.IsIdentical(ezVec3(1.0f)))
    m_Rotation.SetScalingFactors(vScale);
}

inline void ezTransform::SetIdentity()
{
  m_vPosition.SetZero();
  m_Rotation.SetIdentity();
}

inline bool ezTransform::IsIdentical(const ezTransform& rhs) const
{
  return m_vPosition.IsIdentical(rhs.m_vPosition) && m_Rotation.IsIdentical(rhs.m_Rotation);
}

inline bool ezTransform::IsEqual(const ezTransform& rhs, float fEpsilon) const
{
  return m_vPosition.IsEqual(rhs.m_vPosition, fEpsilon) && m_Rotation.IsEqual(rhs.m_Rotation, fEpsilon);
}

inline void ezTransform::SetLocalTransform(const ezTransform& GlobalTransformParent, const ezTransform& GlobalTransformChild)
{
  const ezMat3 mParentInverseRot = GlobalTransformParent.m_Rotation.GetInverse();

  m_vPosition = mParentInverseRot * (GlobalTransformChild.m_vPosition - GlobalTransformParent.m_vPosition);
  m_Rotation  = mParentInverseRot * GlobalTransformChild.m_Rotation;
}

inline void ezTransform::SetGlobalTransform(const ezTransform& GlobalTransformParent, const ezTransform& LocalTransformChild)
{
  m_vPosition = GlobalTransformParent.m_vPosition + (GlobalTransformParent.m_Rotation * LocalTransformChild.m_vPosition);
  m_Rotation = GlobalTransformParent.m_Rotation * LocalTransformChild.m_Rotation;
}

EZ_FORCE_INLINE const ezMat4 ezTransform::GetAsMat4() const
{
  return ezMat4(m_Rotation, m_vPosition);
}

EZ_FORCE_INLINE void ezTransform::operator*=(const ezQuat& q)
{
  m_Rotation = q.GetAsMat3() * m_Rotation;
}

EZ_FORCE_INLINE const ezTransform operator*(const ezQuat& q, const ezTransform& t)
{
  return ezTransform(t.m_vPosition, q.GetAsMat3() * t.m_Rotation);
}

EZ_FORCE_INLINE const ezTransform operator+(const ezTransform& t, const ezVec3& v)
{
  return ezTransform(t.m_vPosition + v, t.m_Rotation);
}

EZ_FORCE_INLINE const ezTransform operator-(const ezTransform& t, const ezVec3& v)
{
  return ezTransform(t.m_vPosition - v, t.m_Rotation);
}

EZ_FORCE_INLINE const ezVec3 operator*(const ezTransform& t, const ezVec3& v)
{
  return (t.m_Rotation * v) + t.m_vPosition;
}

inline const ezTransform operator*(const ezTransform& t1, const ezTransform& t2)
{
  ezTransform r;
  r.m_Rotation = t1.m_Rotation * t2.m_Rotation;
  r.m_vPosition = t1.m_Rotation * t2.m_vPosition + t1.m_vPosition;

  return r;
}

inline ezResult ezTransform::Invert(float fEpsilon)
{
  if (m_Rotation.Invert(fEpsilon).IsFailure())
    return EZ_FAILURE;

  m_vPosition = m_Rotation * -m_vPosition;

  return EZ_SUCCESS;
}

inline const ezTransform ezTransform::GetInverse() const
{
  ezTransform Inverse = *this;
  Inverse.Invert();
  return Inverse;
}

void ezTransform::GetAsArray(float* out_pData, ezMatrixLayout::Enum layout) const
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