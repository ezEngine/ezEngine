#include "../Plane.h"



namespace AE_NS_FOUNDATION
{
  /*! The given vertices can be partially equal or lie on the same line. The algorithm will try to find 3 vertices, that
    form a plane, and deduce the normal from them. This algorithm is much slower, than all the other methods, so only
    use it, when you know, that your data can contain such configurations. */
  void aePlane::CreatePlane (const aeVec3* const pVertices, aeUInt32 iMaxVertices)
  {
    const aeVec3 v1 = pVertices[0];

    aeUInt32 i = 1;
    while (i < iMaxVertices)
    {
      if (pVertices[i].IsEqual (v1, 0.001f) == false)
        break;

      ++i;
    }

    const aeVec3 v2 = pVertices[i];

    const aeVec3 vDir1 = (v1 - v2).GetNormalized ();

    ++i;

    while (i < iMaxVertices)
    {
      if (aeMath::Abs ((pVertices[i] - v2).GetNormalized ().Dot (vDir1)) < 0.999f)
      {
        m_vNormal = (v1 - v2).Cross (v1 - pVertices[i]).GetNormalized ();
        break;
      }

      ++i;
    }
    
    m_fDistance = m_vNormal.Dot (v1);
  }

  bool aePlane::FindSupportPoints (const aeVec3* const pVertices, int iMaxVertices, int& out_v1, int& out_v2, int& out_v3)
  {
    const aeVec3 v1 = pVertices[0];

    bool bFoundSecond = false;

    int i = 1;
    while (i < iMaxVertices)
    {
      if (pVertices[i].IsEqual (v1, 0.001f) == false)
      {
        bFoundSecond = true;
        break;
      }

      ++i;
    }

    if (!bFoundSecond)
      return (false);

    const aeVec3 v2 = pVertices[i];

    const aeVec3 vDir1 = (v1 - v2).GetNormalized ();

    out_v1 = 0;
    out_v2 = i;

    ++i;

    while (i < iMaxVertices)
    {
      // check for inequality, then for non-colinearity
      if ((pVertices[i].IsEqual (v2, 0.001f) == false) && 
        (aeMath::Abs ((pVertices[i] - v2).GetNormalized ().Dot (vDir1)) < 0.999f))
      {
        out_v3 = i;
        return (true);
      }

      ++i;
    }
    
    return (false);
  }

  aePositionOnPlane::Enum aePlane::GetObjectPosition (const aeVec3* const vPoints, int iVertices) const
  {
    bool bFront = false;
    bool bBack = false;

    for (aeInt32 i = 0; i < iVertices; ++i)
    {
      switch (GetPointPosition (vPoints[i]))
      {
      case aePositionOnPlane::Front:
        if (bBack)
          return (aePositionOnPlane::Spanning);
        bFront = true;
        break;
      case aePositionOnPlane::Back:
        if (bFront)
          return (aePositionOnPlane::Spanning);
        bBack = true;
        break;
      }
    }

    return (bFront ? aePositionOnPlane::Front : aePositionOnPlane::Back);
  }


  aePositionOnPlane::Enum aePlane::GetObjectPosition (const aeVec3* const vPoints, int iVertices, float fPlaneHalfWidth) const
  {
    bool bFront = false;
    bool bBack = false;

    for (aeInt32 i = 0; i < iVertices; ++i)
    {
      switch (GetPointPosition (vPoints[i], fPlaneHalfWidth))
      {
      case aePositionOnPlane::Front:
        if (bBack)
          return (aePositionOnPlane::Spanning);
        bFront = true;
        break;
      case aePositionOnPlane::Back:
        if (bFront)
          return (aePositionOnPlane::Spanning);
        bBack = true;
        break;
      }
    }

    if (bFront)
      return (aePositionOnPlane::Front);
    if (bBack)
      return (aePositionOnPlane::Back);

    return (aePositionOnPlane::OnPlane);
  }

  /*! Rays with the origin BEHIND the plane will be ignored (returns false), thus no back-faces are being hit. Also, if the ray points away from the plane.
    the function returns false. If the ray starts in front of the plane and points at it, the function will return true, and return the intersection
    time and point in the out-parameters.
  */
  bool aePlane::GetRayIntersection (const aeVec3& vPointOnRay, const aeVec3& vRayDirectionNormalized, float& out_fIntersectionTime, aeVec3& out_vIntersection) const
  {
    const float fCosAlpha = m_vNormal.Dot (vRayDirectionNormalized);
    const float fDistance = GetDistanceToPoint (vPointOnRay);

    if (fCosAlpha * aeMath::Sign (fDistance) >= 0.0f)	// ray points away from the plane
      return (false);

    //if (fDistance < 0.0f)	// ray starting point is behind the plane (back-faces shall be ignored)
      //return (false);

    out_fIntersectionTime = -fDistance / fCosAlpha;
    out_vIntersection = vPointOnRay + out_fIntersectionTime * vRayDirectionNormalized;

    return (true); 
  }

  bool aePlane::GetInfiniteRayIntersection (const aeVec3& vPointOnRay, const aeVec3& vRayDirectionNormalized, float& out_fIntersectionTime, aeVec3& out_vIntersection) const
  {
    const float fCosAlpha = m_vNormal.Dot (vRayDirectionNormalized);

    if (fCosAlpha == 0.0f)	// ray is parallel to the plane
      return (false);

    const float fDistance = GetDistanceToPoint (vPointOnRay);

    out_fIntersectionTime = -fDistance / fCosAlpha;
    out_vIntersection = vPointOnRay + out_fIntersectionTime * vRayDirectionNormalized;

    return (true); 
  }
}


