#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Math/Vec3.h>

namespace ezIntersectionUtils
{
  /// \brief Checks whether a ray intersects with a triangle.
  ///
  /// The vertex winding order does not matter, triangles will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param vVertex0, vVertex1, vVertex2
  ///   The three vertices forming the triangle.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the triangle. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the triangle.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  ///
  /// \return
  ///   True, if the ray intersects the triangle, false otherwise.
  EZ_FOUNDATION_DLL bool RayTriangleIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDir, const ezVec3& vVertex0, const ezVec3& vVertex1, const ezVec3& vVertex2, float* out_pIntersectionTime = nullptr, ezVec3* out_pIntersectionPoint = nullptr); // [tested]

  /// \brief Checks whether a ray intersects with a polygon.
  ///
  /// The vertex winding order does not matter, polygons will be hit from both sides.
  ///
  /// \param vRayStartPos
  ///   The start position of the ray.
  /// \param vRayDir
  ///   The direction of the ray. This does not need to be normalized. Depending on its length, out_fIntersectionTime will be scaled differently.
  /// \param pPolygonVertices
  ///   Pointer to the first vertex of the polygon.
  /// \param uiNumVertices
  ///   The number of vertices in the polygon.
  /// \param out_fIntersectionTime
  ///   The 'time' at which the ray intersects the polygon. If \a vRayDir is normalized, this is the exact distance.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param out_fIntersectionPoint
  ///   The point where the ray intersects the polygon.
  ///   out_fIntersectionPoint == vRayStartPos + vRayDir * out_fIntersectionTime
  ///   This parameter is optional and may be set to nullptr.
  /// \param uiVertexStride
  ///   The stride in bytes between each vertex in the pPolygonVertices array. If the array is tightly packed, this will equal sizeof(ezVec3), but it
  ///   can be larger, if the vertices are interleaved with other data.
  /// \return
  ///   True, if the ray intersects the polygon, false otherwise.
  EZ_FOUNDATION_DLL bool RayPolygonIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDir, const ezVec3* pPolygonVertices,
    ezUInt32 uiNumVertices, float* out_pIntersectionTime = nullptr, ezVec3* out_pIntersectionPoint = nullptr,
    ezUInt32 uiVertexStride = sizeof(ezVec3)); // [tested]


  /// \brief Returns point on the line segment that is closest to \a vStartPoint. Optionally also returns the fraction along the segment, where that
  /// point is located.
  EZ_FOUNDATION_DLL ezVec3 ClosestPoint_PointLineSegment(const ezVec3& vStartPoint, const ezVec3& vLineSegmentPos0, const ezVec3& vLineSegmentPos1,
    float* out_pFractionAlongSegment = nullptr); // [tested]

  /// \brief Computes the intersection point and time of the 2D ray with the 2D line segment. Returns true, if there is an intersection.
  EZ_FOUNDATION_DLL bool Ray2DLine2D(const ezVec2& vRayStartPos, const ezVec2& vRayDir, const ezVec2& vLineSegmentPos0,
    const ezVec2& vLineSegmentPos1, float* out_pIntersectionTime = nullptr, ezVec2* out_pIntersectionPoint = nullptr); // [tested]

  /// \brief Tests whether a point is located on a line
  EZ_FOUNDATION_DLL bool IsPointOnLine(const ezVec3& vLineStart, const ezVec3& vLineEnd, const ezVec3& vPoint, float fMaxDist = 0.01f);

} // namespace ezIntersectionUtils
