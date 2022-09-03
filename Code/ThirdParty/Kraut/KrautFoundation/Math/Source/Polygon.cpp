#include "../Vec3.h"
#include "../Plane.h"



namespace AE_NS_FOUNDATION
{
  static bool PointIn2DPolyXY (int nvert, const aeVec3* pVertices, const aeVec3& pPoint)
  {
    bool c = false;

    int j = (nvert - 1); // last vertex

    for (int i = 0; i < nvert; ++i) 
    {
      if (((pVertices[i].y > pPoint.y) != (pVertices[j].y > pPoint.y)) &&
        (pPoint.x < (pVertices[j].x - pVertices[i].x) * (pPoint.y - pVertices[i].y) / (pVertices[j].y - pVertices[i].y) + pVertices[i].x))
      {
        c = !c;
      }

      j = i;
    }

    return (c);
  }

  static bool PointIn2DPolyXZ (int nvert, const aeVec3* pVertices, const aeVec3& pPoint)
  {
    bool c = false;

    int j = (nvert - 1); // last vertex

    for (int i = 0; i < nvert; ++i) 
    {
      if (((pVertices[i].z > pPoint.z) != (pVertices[j].z > pPoint.z)) &&
        (pPoint.x < (pVertices[j].x - pVertices[i].x) * (pPoint.z - pVertices[i].z) / (pVertices[j].z - pVertices[i].z) + pVertices[i].x))
      {
        c = !c;
      }

      j = i;
    }

    return (c);
  }

  static bool PointIn2DPolyZY (int nvert, const aeVec3* pVertices, const aeVec3& pPoint)
  {
    bool c = false;

    int j = (nvert - 1); // last vertex

    for (int i = 0; i < nvert; ++i) 
    {
      if (((pVertices[i].y > pPoint.y) != (pVertices[j].y > pPoint.y)) &&
        (pPoint.z < (pVertices[j].z - pVertices[i].z) * (pPoint.y - pVertices[i].y) / (pVertices[j].y - pVertices[i].y) + pVertices[i].z))
      {
        c = !c;
      }

      j = i;
    }

    return (c);
  }

  bool getInfiniteRayPolygonIntersection (const aeVec3* pVertices, aeUInt32 uiVertices, const aeVec3& vRayStart, const aeVec3& vRayDirNorm, float& out_fIntersectionTime, aeVec3& out_vIntersection)
  {
    aePlane Plane (pVertices, uiVertices);

    if (!Plane.GetInfiniteRayIntersection (vRayStart, vRayDirNorm, out_fIntersectionTime, out_vIntersection))
      return (false);


    if (aeMath::Abs (Plane.m_vNormal.Dot (aeVec3 (0.0f, 1.0f, 0.0f))) > 0.6f)
      return (PointIn2DPolyXZ (uiVertices, pVertices, out_vIntersection));

    if (aeMath::Abs (Plane.m_vNormal.Dot (aeVec3 (1.0f, 0.0f, 0.0f))) > 0.6f)
      return (PointIn2DPolyZY (uiVertices, pVertices, out_vIntersection));

    return (PointIn2DPolyXY (uiVertices, pVertices, out_vIntersection));
  }

  bool aeMath::GetRayPolygonIntersection (const aeVec3* pVertices, aeUInt32 uiVertices, const aeVec3& vRayStart, const aeVec3& vRayDirNorm, float& out_fIntersectionTime, aeVec3& out_vIntersection)
  {
    aePlane Plane (pVertices, uiVertices);

    // ignore back faces
    if (Plane.GetPointPosition (vRayStart) != aePositionOnPlane::Front)
      return false;

    if (!Plane.GetRayIntersection (vRayStart, vRayDirNorm, out_fIntersectionTime, out_vIntersection))
      return (false);


    if (aeMath::Abs (Plane.m_vNormal.Dot (aeVec3 (0.0f, 1.0f, 0.0f))) > 0.6f)
      return (PointIn2DPolyXZ (uiVertices, pVertices, out_vIntersection));

    if (aeMath::Abs (Plane.m_vNormal.Dot (aeVec3 (1.0f, 0.0f, 0.0f))) > 0.6f)
      return (PointIn2DPolyZY (uiVertices, pVertices, out_vIntersection));

    return (PointIn2DPolyXY (uiVertices, pVertices, out_vIntersection));
  }


  bool aeMath::IsPointInPolygon (const aeVec3* pVertices, aeUInt32 uiVertices, const aeVec3& vPoint)
  {
    aePlane Plane (pVertices, uiVertices);

    if (aeMath::Abs (Plane.m_vNormal.Dot (aeVec3 (0.0f, 1.0f, 0.0f))) > 0.6f)
      return (PointIn2DPolyXZ (uiVertices, pVertices, vPoint));

    if (aeMath::Abs (Plane.m_vNormal.Dot (aeVec3 (1.0f, 0.0f, 0.0f))) > 0.6f)
      return (PointIn2DPolyZY (uiVertices, pVertices, vPoint));

    return (PointIn2DPolyXY (uiVertices, pVertices, vPoint));
  }
}