#include <Foundation/PCH.h>
#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Plane.h>

bool ezIntersectionUtils::RayPolygonIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDir, const ezVec3* pPolygonVertices, ezUInt32 uiNumVertices, float* out_fIntersectionTime, ezVec3* out_vIntersectionPoint, ezUInt32 uiVertexStride)
{
  EZ_ASSERT_DEBUG(uiNumVertices >= 3, "A polygon must have at least three vertices.");
  EZ_ASSERT_DEBUG(uiVertexStride >= sizeof(ezVec3), "The vertex stride is invalid.");

  ezPlane p(*pPolygonVertices, *ezMemoryUtils::AddByteOffsetConst<ezVec3>(pPolygonVertices, uiVertexStride), *ezMemoryUtils::AddByteOffsetConst<ezVec3>(pPolygonVertices, uiVertexStride * 2));

  EZ_ASSERT_DEBUG(p.IsValid(), "The given polygon's plane is invalid (computed from the first three vertices only).");

  ezVec3 vIntersection;

  if (!p.GetRayIntersection(vRayStartPos, vRayDir, out_fIntersectionTime, &vIntersection))
    return false;

  if (out_vIntersectionPoint)
    *out_vIntersectionPoint = vIntersection;

  // start with the last point as the 'wrap around' position
  ezVec3 vPrevPoint = *ezMemoryUtils::AddByteOffsetConst<ezVec3>(pPolygonVertices, uiVertexStride * (uiNumVertices - 1));

  // for each polygon edge
  for (ezUInt32 i = 0; i < uiNumVertices; ++i)
  {
    const ezVec3 vThisPoint = *ezMemoryUtils::AddByteOffsetConst<ezVec3>(pPolygonVertices, uiVertexStride * i);

    const ezPlane EdgePlane(vThisPoint, vPrevPoint, vPrevPoint + p.m_vNormal);

    // if the intersection point is outside of any of the edge planes, it is not inside the (convex) polygon
    if (EdgePlane.GetPointPosition(vIntersection) == ezPositionOnPlane::Back)
      return false;

    vPrevPoint = vThisPoint;
  }

  // inside all edge planes -> inside the polygon -> there is a proper intersection
  return true;
}

ezVec3 ezIntersectionUtils::ClosestPoint_PointLineSegment(const ezVec3& vStartPoint, const ezVec3& vLineSegmentPos0, const ezVec3& vLineSegmentPos1, float* out_fFractionAlongSegment)
{
  const ezVec3 vLineDir = vLineSegmentPos1 - vLineSegmentPos0;
  const ezVec3 vToStartPoint = vStartPoint - vLineSegmentPos0;

  const float fProjected = vToStartPoint.Dot(vLineDir);

  float fPosAlongSegment;

  // clamp t to [0; 1] range, and only do the division etc. when necessary
  if (fProjected <= 0.0f)
  {
    fPosAlongSegment = 0.0f;
  }
  else
  {
    const float fSquaredDirLen = vLineDir.GetLengthSquared();

    if (fProjected >= fSquaredDirLen)
    {
      fPosAlongSegment = 1.0f;
    }
    else
    {
      fPosAlongSegment = fProjected / fSquaredDirLen;
    }
  }

  if (out_fFractionAlongSegment)
    *out_fFractionAlongSegment = fPosAlongSegment;

  return vLineSegmentPos0 + fPosAlongSegment * vLineDir;
}

bool ezIntersectionUtils::Ray2DLine2D(const ezVec2& vRayStartPos, const ezVec2& vRayDir, const ezVec2& vLineSegmentPos0, const ezVec2& vLineSegmentPos1, float* out_fIntersectionTime, ezVec2* out_vIntersectionPoint)
{
  const ezVec2 vLineDir = vLineSegmentPos1 - vLineSegmentPos0;

  // 2D Plane
  const ezVec2 vPlaneNormal = vLineDir.GetOrthogonalVector();
  const float fPlaneNegDist = -vPlaneNormal.Dot(vLineSegmentPos0);

  ezVec2 vIntersection;
  float fIntersectionTime;

  // 2D Plane ray intersection test
  {
    const float fPlaneSide = vPlaneNormal.Dot(vRayStartPos) + fPlaneNegDist;
    const float fCosAlpha = vPlaneNormal.Dot(vRayDir);

    if (fCosAlpha == 0)  // ray is orthogonal to plane
      return false;

    if (ezMath::Sign(fPlaneSide) == ezMath::Sign(fCosAlpha)) // ray points away from the plane
      return false;

    fIntersectionTime = -fPlaneSide / fCosAlpha;

    vIntersection = vRayStartPos + fIntersectionTime * vRayDir;
  }

  const ezVec2 vToIntersection = vIntersection - vLineSegmentPos0;

  const float fProjected = vLineDir.Dot(vToIntersection);

  if (fProjected < 0.0f)
    return false;

  if (fProjected > vLineDir.GetLengthSquared())
    return false;

  if (out_fIntersectionTime)
    *out_fIntersectionTime = fIntersectionTime;

  if (out_vIntersectionPoint)
    *out_vIntersectionPoint = vIntersection;

  return true;
}



EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Intersection);

