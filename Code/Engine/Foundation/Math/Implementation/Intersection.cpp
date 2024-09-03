#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Intersection.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Plane.h>

bool ezIntersectionUtils::RayTriangleIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDir, const ezVec3& vVertex0, const ezVec3& vVertex1, const ezVec3& vVertex2, float* out_pIntersectionTime /*= nullptr*/, ezVec3* out_pIntersectionPoint /*= nullptr*/)
{
  const ezPlane plane = ezPlane::MakeFromPoints(vVertex0, vVertex1, vVertex2);

  ezVec3 vIntersection;

  if (!plane.GetRayIntersection(vRayStartPos, vRayDir, out_pIntersectionTime, &vIntersection))
    return false;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  {
    const ezVec3 edge = vVertex1 - vVertex0;
    const ezVec3 vp = vIntersection - vVertex0;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }
  }

  {
    const ezVec3 edge = vVertex2 - vVertex1;
    const ezVec3 vp = vIntersection - vVertex1;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }
  }

  {
    const ezVec3 edge = vVertex0 - vVertex2;
    const ezVec3 vp = vIntersection - vVertex2;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }
  }

  return true;
}

bool ezIntersectionUtils::RayPolygonIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDir, const ezVec3* pPolygonVertices,
  ezUInt32 uiNumVertices, float* out_pIntersectionTime, ezVec3* out_pIntersectionPoint, ezUInt32 uiVertexStride)
{
  EZ_ASSERT_DEBUG(uiNumVertices >= 3, "A polygon must have at least three vertices.");
  EZ_ASSERT_DEBUG(uiVertexStride >= sizeof(ezVec3), "The vertex stride is invalid.");

  ezPlane plane = ezPlane::MakeFromPoints(*pPolygonVertices, *ezMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride), *ezMemoryUtils::AddByteOffset(pPolygonVertices, uiVertexStride * 2));

  EZ_ASSERT_DEBUG(plane.IsValid(), "The given polygon's plane is invalid (computed from the first three vertices only).");

  ezVec3 vIntersection;

  if (!plane.GetRayIntersection(vRayStartPos, vRayDir, out_pIntersectionTime, &vIntersection))
    return false;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  // start with the last point as the 'wrap around' position
  ezVec3 vPrevPoint = *ezMemoryUtils::AddByteOffset(pPolygonVertices, ezMath::SafeMultiply32(uiVertexStride, (uiNumVertices - 1)));

  // for each polygon edge
  for (ezUInt32 i = 0; i < uiNumVertices; ++i)
  {
    const ezVec3 vThisPoint = *ezMemoryUtils::AddByteOffset(pPolygonVertices, ezMath::SafeMultiply32(uiVertexStride, i));

    const ezVec3 edge = vThisPoint - vPrevPoint;
    const ezVec3 vp = vIntersection - vPrevPoint;
    if (plane.m_vNormal.Dot(edge.CrossRH(vp)) < 0)
    {
      return false;
    }

    vPrevPoint = vThisPoint;
  }

  // inside all edge planes -> inside the polygon -> there is a proper intersection
  return true;
}

ezVec3 ezIntersectionUtils::ClosestPoint_PointLineSegment(
  const ezVec3& vStartPoint, const ezVec3& vLineSegmentPos0, const ezVec3& vLineSegmentPos1, float* out_pFractionAlongSegment)
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

  if (out_pFractionAlongSegment)
    *out_pFractionAlongSegment = fPosAlongSegment;

  return vLineSegmentPos0 + fPosAlongSegment * vLineDir;
}

bool ezIntersectionUtils::Ray2DLine2D(const ezVec2& vRayStartPos, const ezVec2& vRayDir, const ezVec2& vLineSegmentPos0,
  const ezVec2& vLineSegmentPos1, float* out_pIntersectionTime, ezVec2* out_pIntersectionPoint)
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

    if (fCosAlpha == 0)                                      // ray is orthogonal to plane
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

  if (out_pIntersectionTime)
    *out_pIntersectionTime = fIntersectionTime;

  if (out_pIntersectionPoint)
    *out_pIntersectionPoint = vIntersection;

  return true;
}

bool ezIntersectionUtils::IsPointOnLine(const ezVec3& vLineStart, const ezVec3& vLineEnd, const ezVec3& vPoint, float fMaxDist /*= 0.01f*/)
{
  const ezVec3 vClosest = ClosestPoint_PointLineSegment(vPoint, vLineStart, vLineEnd);
  const float fClosestDistSqr = (vClosest - vPoint).GetLengthSquared();

  return (fClosestDistSqr <= fMaxDist * fMaxDist);
}


