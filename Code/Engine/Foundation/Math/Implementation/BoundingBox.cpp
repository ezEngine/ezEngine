#include <Foundation/PCH.h>
#include <Foundation/Math/BoundingBox.h>


bool ezBoundingBox::GetRayIntersection(const ezVec3& vStartPos, const ezVec3& vRayDir, float* out_fIntersection, ezVec3* out_vIntersection) const
{
  EZ_ASSERT(vStartPos.IsValid(), "Ray start position must be valid.");
  EZ_ASSERT(vRayDir.IsValid(), "Ray direction must be valid.");

  const ezVec3 vDirFrac = ezVec3(1.0f).CompDiv(vRayDir);

  const float t1 = (m_vMin.x - vStartPos.x) * vDirFrac.x;
  const float t2 = (m_vMax.x - vStartPos.x) * vDirFrac.x;
  const float t3 = (m_vMin.y - vStartPos.y) * vDirFrac.y;
  const float t4 = (m_vMax.y - vStartPos.y) * vDirFrac.y;
  const float t5 = (m_vMin.z - vStartPos.z) * vDirFrac.z;
  const float t6 = (m_vMax.z - vStartPos.z) * vDirFrac.z;

  const float tmin = ezMath::Max(ezMath::Min(t1, t2), ezMath::Min(t3, t4), ezMath::Min(t5, t6));
  const float tmax = ezMath::Min(ezMath::Max(t1, t2), ezMath::Max(t3, t4), ezMath::Max(t5, t6));

  // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
  if (tmax <= 0)
    return false;

  // if tmin > tmax, ray doesn't intersect AABB
  if (tmin > tmax)
    return false;

  if (out_fIntersection)
    *out_fIntersection = tmin;

  if (out_vIntersection)
    *out_vIntersection = vStartPos + tmin * vRayDir;

  return true;
}

bool ezBoundingBox::GetLineSegmentIntersection(const ezVec3& vStartPos, const ezVec3& vEndPos, float* out_fLineFraction, ezVec3* out_vIntersection) const
{
  const ezVec3 vRayDir = vEndPos - vStartPos;

  float fIntersection = 0.0f;
  if (!GetRayIntersection(vStartPos, vRayDir, &fIntersection, out_vIntersection))
    return false;

  if (out_fLineFraction)
    *out_fLineFraction = fIntersection;

  return fIntersection <= 1.0f;
}

