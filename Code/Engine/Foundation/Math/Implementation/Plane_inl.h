#pragma once

#include <Foundation/Math/Mat4.h>

EZ_FORCE_INLINE ezPlane::ezPlane()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_vNormal.Set(ezMath::NaN());
  m_fNegDistance = ezMath::NaN();
#endif
}

inline ezPlane::ezPlane(const ezVec3 &vNormal, const ezVec3& vPointOnPlane)
{
  SetFromNormalAndPoint(vNormal, vPointOnPlane);
}

inline ezPlane::ezPlane(const ezVec3& v1, const ezVec3& v2, const ezVec3& v3)
{
  SetFromPoints(v1, v2, v3);
}

inline ezPlane::ezPlane(const ezVec3* const pVertices)
{
  SetFromPoints(pVertices);
}

inline ezPlane::ezPlane(const ezVec3* const pVertices, ezUInt32 iMaxVertices)
{
  SetFromPoints(pVertices, iMaxVertices);
}

inline void ezPlane::SetFromNormalAndPoint(const ezVec3 &vNormal, const ezVec3& vPointOnPlane)
{
  EZ_ASSERT(vNormal.IsNormalized(), "Normal must be normalized.");

  m_vNormal = vNormal; 
  m_fNegDistance = -m_vNormal.Dot(vPointOnPlane);
}

inline ezResult ezPlane::SetFromPoints(const ezVec3& v1, const ezVec3& v2, const ezVec3& v3)
{
  if (m_vNormal.CalculateNormal(v1, v2, v3) == EZ_FAILURE)
    return EZ_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(v1);
  return EZ_SUCCESS;
}

inline ezResult ezPlane::SetFromPoints(const ezVec3* const pVertices)
{
  if (m_vNormal.CalculateNormal(pVertices[0], pVertices[1], pVertices[2]) == EZ_FAILURE)
    return EZ_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(pVertices[0]);
  return EZ_SUCCESS;
}

inline ezResult ezPlane::SetFromDirections(const ezVec3& vTangent1, const ezVec3& vTangent2, const ezVec3& vPointOnPlane)
{
  ezVec3 vNormal = vTangent1.Cross(vTangent2);
  ezResult res = vNormal.NormalizeIfNotZero();

  m_vNormal  =  vNormal;
  m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

inline void ezPlane::Transform(const ezMat3& m)
{
  ezVec3 vPointOnPlane = m_vNormal * -m_fNegDistance;

  // rotate the normal, translate the point
  SetFromNormalAndPoint(m.TransformDirection(m_vNormal), m * vPointOnPlane);
}

inline void ezPlane::Transform(const ezMat4& m)
{
  ezVec3 vPointOnPlane = m_vNormal * -m_fNegDistance;

  // rotate the normal, translate the point
  SetFromNormalAndPoint(m.TransformDirection(m_vNormal), m * vPointOnPlane);
}

EZ_FORCE_INLINE void ezPlane::Flip()
{
  m_fNegDistance = -m_fNegDistance;
  m_vNormal = -m_vNormal;
}

EZ_FORCE_INLINE float ezPlane::GetDistanceTo(const ezVec3& vPoint) const
{
  return (m_vNormal.Dot (vPoint) + m_fNegDistance);
}

EZ_FORCE_INLINE ezPositionOnPlane::Enum ezPlane::GetPointPosition(const ezVec3& vPoint) const
{
  return (m_vNormal.Dot(vPoint) < -m_fNegDistance ? ezPositionOnPlane::Back : ezPositionOnPlane::Front);
}

inline ezPositionOnPlane::Enum ezPlane::GetPointPosition(const ezVec3& vPoint, float fPlaneHalfWidth) const
{
  const float f = m_vNormal.Dot(vPoint);

  if (f + fPlaneHalfWidth < -m_fNegDistance)
    return ezPositionOnPlane::Back;

  if (f - fPlaneHalfWidth > -m_fNegDistance)
    return ezPositionOnPlane::Front;

  return ezPositionOnPlane::OnPlane;
}

EZ_FORCE_INLINE const ezVec3 ezPlane::ProjectOntoPlane(const ezVec3& vPoint) const
{
  return vPoint - m_vNormal * (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

EZ_FORCE_INLINE const ezVec3 ezPlane::Mirror(const ezVec3& vPoint) const
{
  return vPoint - 2.0f * GetDistanceTo(vPoint) * m_vNormal;
}

inline const ezVec3 ezPlane::GetCoplanarDirection(const ezVec3& vDirection) const
{
  ezVec3 res = vDirection;
  res.MakeOrthogonalTo(m_vNormal);
  return res;
}

inline bool ezPlane::IsIdentical(const ezPlane& rhs) const
{
  return m_vNormal.IsIdentical(rhs.m_vNormal) && m_fNegDistance == rhs.m_fNegDistance;
}

inline bool ezPlane::IsEqual(const ezPlane& rhs, float fEpsilon) const
{
  return m_vNormal.IsEqual(rhs.m_vNormal, fEpsilon) && ezMath::IsFloatEqual(m_fNegDistance, rhs.m_fNegDistance, fEpsilon);
}

EZ_FORCE_INLINE bool operator== (const ezPlane& lhs, const ezPlane& rhs)
{
  return lhs.IsIdentical(rhs);
}

EZ_FORCE_INLINE bool operator!= (const ezPlane& lhs, const ezPlane& rhs)
{
  return !lhs.IsIdentical(rhs);
}

inline bool ezPlane::FlipIfNecessary(const ezVec3& vPoint, bool bPlaneShouldFacePoint)
{
  if ((GetPointPosition(vPoint) == ezPositionOnPlane::Front) != bPlaneShouldFacePoint)
  {
    Flip();
    return true;
  }

  return false;
}

inline void ezPlane::SetInvalid()
{
  m_vNormal.Set(0.0f);
  m_fNegDistance = 0.0f;
}

inline bool ezPlane::IsValid() const
{
  return ezMath::IsFinite(m_fNegDistance) && m_vNormal.IsNormalized(ezMath_DefaultEpsilon);
}

