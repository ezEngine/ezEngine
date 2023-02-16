#ifndef AE_FOUNDATION_MATHS_PLANE_INL
#define AE_FOUNDATION_MATHS_PLANE_INL

#include "../Matrix.h"

namespace AE_NS_FOUNDATION
{
  inline aePlane::aePlane (const aeVec3 &vNormal, float fDistToPlane)
  {
    CreatePlane (vNormal, fDistToPlane);
  }

  inline aePlane::aePlane (const aeVec3 &vNormal, const aeVec3& vPointOnPlane)
  {
    CreatePlane (vNormal, vPointOnPlane);
  }

  inline aePlane::aePlane (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3)
  {
    CreatePlane (v1, v2, v3);
  }

  inline aePlane::aePlane (const aeVec3* const pVertices)
  {
    CreatePlane (pVertices);
  }

  inline aePlane::aePlane (const aeVec3* const pVertices, aeUInt32 iMaxVertices)
  {
    CreatePlane (pVertices, iMaxVertices);
  }

  inline void aePlane::CreatePlane (const aeVec3 &vNormal, const aeVec3& vPointOnPlane)
  {
    m_vNormal = vNormal; 
    m_fDistance = m_vNormal.Dot (vPointOnPlane);
  }

  inline void aePlane::CreatePlane (const aeVec3 &vNormal, float fDistToPlane)
  {
    m_vNormal = vNormal;
    m_fDistance = fDistToPlane;
  }

  inline void aePlane::CreatePlane (const aeVec3& v1, const aeVec3& v2, const aeVec3& v3)
  {
    m_vNormal.CalculateNormal (v1, v2, v3);
    m_fDistance = m_vNormal.Dot (v1);
  }

  inline void aePlane::CreatePlane (const aeVec3* const pVertices)
  {
    m_vNormal.CalculateNormal (pVertices[0], pVertices[1], pVertices[2]);
    m_fDistance = m_vNormal.Dot (pVertices[0]);
  }

  inline void aePlane::TransformPlane (const aeMatrix& m)
  {
    aeVec3 vPointOnPlane = m_vNormal * m_fDistance;

    // rotate the normal, translate the point
    CreatePlane (m.TransformDirection (m_vNormal), m * vPointOnPlane);
  }

  inline void aePlane::FlipPlane (void)
  {
    m_fDistance = -m_fDistance;
    m_vNormal = -m_vNormal;
  }

  inline float aePlane::GetDistanceToPoint (const aeVec3& vPoint) const
  {
    return (m_vNormal.Dot (vPoint) - m_fDistance);
  }

  inline aePositionOnPlane::Enum aePlane::GetPointPosition (const aeVec3& vPoint) const
  {
    return (m_vNormal.Dot (vPoint) < m_fDistance ? aePositionOnPlane::Back : aePositionOnPlane::Front);
  }

  inline aePositionOnPlane::Enum aePlane::GetPointPosition (const aeVec3& vPoint, float fPlaneHalfWidth) const
  {
    const float f = m_vNormal.Dot (vPoint);

    if (f + fPlaneHalfWidth < m_fDistance)
      return (aePositionOnPlane::Back);

    if (f - fPlaneHalfWidth > m_fDistance)
      return (aePositionOnPlane::Front);

    return (aePositionOnPlane::OnPlane);
  }

  inline const aeVec3 aePlane::ProjectOntoPlane (const aeVec3& vPoint) const
  {
    return vPoint - m_vNormal * (m_vNormal.Dot (vPoint) - m_fDistance);
  }
}

#endif

