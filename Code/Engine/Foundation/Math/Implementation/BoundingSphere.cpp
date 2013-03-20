#include <Foundation/PCH.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/BoundingBox.h>

void ezBoundingSphere::ExpandToInclude(const ezBoundingBox& rhs)
{
  // This could be made more efficient by chosing only the point that is farthest away from the sphere

  ezVec3 vCorners[8];
  rhs.GetCorners(vCorners);

  ExpandToInclude(vCorners, 8);
}

float ezBoundingSphere::GetDistanceTo(const ezBoundingBox& rhs) const
{
  const ezVec3 vPointOnBox = rhs.GetClampedPoint(m_vCenter);

  return GetDistanceTo(vPointOnBox);
}

bool ezBoundingSphere::Contains(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */) const
{
  EZ_ASSERT(pPoints != NULL, "The array must not be empty.");
  EZ_ASSERT(uiNumPoints > 0, "The array must contain at least one point.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "The data must not overlap.");

  const float fRadiusSQR = ezMath::Square(m_fRadius);

  const ezVec3* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() > fRadiusSQR)
      return false;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return true;
}

bool ezBoundingSphere::Contains(const ezBoundingBox& rhs) const
{
  /// \todo This could be done more efficiently.

  ezVec3 vCorners[8];
  rhs.GetCorners(vCorners);

  return Contains(vCorners, 8);
}

bool ezBoundingSphere::Overlaps(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */) const
{
  EZ_ASSERT(pPoints != NULL, "The array must not be empty.");
  EZ_ASSERT(uiNumPoints > 0, "The array must contain at least one point.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "The data must not overlap.");

  const float fRadiusSQR = ezMath::Square(m_fRadius);

  const ezVec3* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() <= fRadiusSQR)
      return true;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return false;
}

bool ezBoundingSphere::Overlaps(const ezBoundingBox& rhs) const
{
  return Contains(rhs.GetClampedPoint(m_vCenter));
}

const ezBoundingBox ezBoundingSphere::GetBoundingBox() const
{
  return ezBoundingBox(m_vCenter - ezVec3(m_fRadius), m_vCenter + ezVec3(m_fRadius));
}

void ezBoundingSphere::SetFromPoints(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */)
{
  EZ_ASSERT(pPoints != NULL, "The array must not be empty.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "The data must not overlap.");
  EZ_ASSERT(uiNumPoints > 0, "The array must contain at least one point.");

  const ezVec3* pCur = &pPoints[0];

  ezVec3 vCenter(0.0f);

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  vCenter /= (float) uiNumPoints;

  float fMaxDistSQR = 0.0f;

  pCur = &pPoints[0];
  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    fMaxDistSQR = (*pCur - vCenter).GetLengthSquared();
    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  m_vCenter = vCenter;
  m_fRadius = ezMath::Sqrt(fMaxDistSQR);

  EZ_ASSERT(IsValid(), "The point cloud contained corrupted data.");
}

void ezBoundingSphere::ExpandToInclude(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */)
{
  EZ_ASSERT(pPoints != NULL, "The array must not be empty.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "The data must not overlap.");

  const ezVec3* pCur = &pPoints[0];

  float fMaxDistSQR = 0.0f;

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const float fDistSQR = (*pCur - m_vCenter).GetLengthSquared();

    fMaxDistSQR = ezMath::Max(fMaxDistSQR, fDistSQR);

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  if (ezMath::Square(m_fRadius) < fMaxDistSQR)
    m_fRadius = ezMath::Sqrt(fMaxDistSQR);
}

float ezBoundingSphere::GetDistanceTo(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */) const
{
  EZ_ASSERT(pPoints != NULL, "The array must not be empty.");
  EZ_ASSERT(uiNumPoints > 0, "The array must contain at least one point.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "The data must not overlap.");

  const ezVec3* pCur = &pPoints[0];

  float fMinDistSQR = ezMath::FloatMax_Pos();

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const float fDistSQR = (*pCur - m_vCenter).GetLengthSquared();

    fMinDistSQR = ezMath::Min(fMinDistSQR, fDistSQR);

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return ezMath::Sqrt(fMinDistSQR);
}

bool ezBoundingSphere::GetRayIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDirNormalized, float* out_fIntersection /* = NULL */, ezVec3* out_vIntersection /* = NULL */) const
{
  EZ_ASSERT(vRayDirNormalized.IsNormalized(), "The ray direction must be normalized.");

  // Ugly Code taken from 'Real Time Rendering First Edition' Page 299

  const float fRadiusSQR = ezMath::Square(m_fRadius);
  const ezVec3 vRelPos = m_vCenter - vRayStartPos;

  const float d = vRelPos.Dot(vRayDirNormalized);
  const float fRelPosLenSQR = vRelPos.GetLengthSquared();

  if (d < 0.0f && fRelPosLenSQR > fRadiusSQR)
    return false;

  const float m2 = fRelPosLenSQR - ezMath::Square(d);

  if (m2 > fRadiusSQR)
    return false;

  const float q = ezMath::Sqrt(fRadiusSQR - m2);

  float fIntersectionTime;

  if (fRelPosLenSQR > fRadiusSQR)
    fIntersectionTime = d - q;
  else
    fIntersectionTime = d + q;

  if (out_fIntersection)
    *out_fIntersection = fIntersectionTime;
  if (out_vIntersection)
    *out_vIntersection = vRayStartPos + vRayDirNormalized * fIntersectionTime;

  return true;
}

bool ezBoundingSphere::GetLineSegmentIntersection(const ezVec3& vLineStartPos, const ezVec3& vLineEndPos, float* out_fHitFraction /* = NULL */, ezVec3* out_vIntersection /* = NULL */) const
{
  float fIntersection = 0.0f;

  const ezVec3 vDir = vLineEndPos - vLineStartPos;
  ezVec3 vDirNorm = vDir;
  const float fLen = vDirNorm.GetLengthAndNormalize ();

  if (!GetRayIntersection (vLineStartPos, vDirNorm, &fIntersection))
    return false;

  if (fIntersection > fLen)
    return false;

  if (out_fHitFraction)
    *out_fHitFraction = fIntersection / fLen;

  if (out_vIntersection)
    *out_vIntersection = vLineStartPos + vDirNorm * fIntersection;

  return true;
}




