#include <Foundation/PCH.h>
#include <Foundation/Math/Plane.h>

/*! The given vertices can be partially equal or lie on the same line. The algorithm will try to find 3 vertices, that
  form a plane, and deduce the normal from them. This algorithm is much slower, than all the other methods, so only
  use it, when you know, that your data can contain such configurations. */
ezResult ezPlane::SetFromPoints(const ezVec3* const pVertices, ezUInt32 iMaxVertices)
{
  ezInt32 iPoints[3];

  if (FindSupportPoints(pVertices, iMaxVertices, iPoints[0], iPoints[1], iPoints[2]) == EZ_FAILURE)
  {
    SetFromPoints(pVertices);
    return EZ_FAILURE;
  }

  SetFromPoints(pVertices[iPoints[0]], pVertices[iPoints[1]], pVertices[iPoints[2]]);
  return EZ_SUCCESS;
}

ezResult ezPlane::FindSupportPoints (const ezVec3* const pVertices, int iMaxVertices, int& out_v1, int& out_v2, int& out_v3)
{
  const ezVec3 v1 = pVertices[0];

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
    return EZ_FAILURE;

  const ezVec3 v2 = pVertices[i];

  const ezVec3 vDir1 = (v1 - v2).GetNormalized();

  out_v1 = 0;
  out_v2 = i;

  ++i;

  while (i < iMaxVertices)
  {
    // check for inequality, then for non-colinearity
    if ((pVertices[i].IsEqual (v2, 0.001f) == false) && 
      (ezMath::Abs ((pVertices[i] - v2).GetNormalized().Dot (vDir1)) < 0.999f))
    {
      out_v3 = i;
      return EZ_SUCCESS;
    }

    ++i;
  }
    
  return EZ_FAILURE;
}

ezPositionOnPlane::Enum ezPlane::GetObjectPosition (const ezVec3* const vPoints, int iVertices) const
{
  bool bFront = false;
  bool bBack = false;

  for (ezInt32 i = 0; i < iVertices; ++i)
  {
    switch (GetPointPosition (vPoints[i]))
    {
    case ezPositionOnPlane::Front:
      if (bBack)
        return (ezPositionOnPlane::Spanning);
      bFront = true;
      break;
    case ezPositionOnPlane::Back:
      if (bFront)
        return (ezPositionOnPlane::Spanning);
      bBack = true;
      break;
    }
  }

  return (bFront ? ezPositionOnPlane::Front : ezPositionOnPlane::Back);
}

ezPositionOnPlane::Enum ezPlane::GetObjectPosition (const ezVec3* const vPoints, int iVertices, float fPlaneHalfWidth) const
{
  bool bFront = false;
  bool bBack = false;

  for (ezInt32 i = 0; i < iVertices; ++i)
  {
    switch (GetPointPosition (vPoints[i], fPlaneHalfWidth))
    {
    case ezPositionOnPlane::Front:
      if (bBack)
        return (ezPositionOnPlane::Spanning);
      bFront = true;
      break;
    case ezPositionOnPlane::Back:
      if (bFront)
        return (ezPositionOnPlane::Spanning);
      bBack = true;
      break;
    }
  }

  if (bFront)
    return (ezPositionOnPlane::Front);
  if (bBack)
    return (ezPositionOnPlane::Back);

  return (ezPositionOnPlane::OnPlane);
}

bool ezPlane::GetRayIntersection(const ezVec3& vRayStartPos, const ezVec3& vRayDir, float* out_fIntersection, ezVec3* out_vIntersection) const
{
  EZ_ASSERT(vRayStartPos.IsValid(), "Ray start position must be valid.");
  EZ_ASSERT(vRayDir.IsValid(), "Ray direction must be valid.");

  const float fPlaneSide = GetDistanceTo(vRayStartPos);
  const float fCosAlpha = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0.0f)  // ray is orthogonal to plane
    return false;

  if (ezMath::Sign(fPlaneSide) == ezMath::Sign(fCosAlpha)) // ray points away from the plane
    return false;

  const float fTime = -fPlaneSide / fCosAlpha;

  if (out_fIntersection)
    *out_fIntersection = fTime;

  if (out_vIntersection)
    *out_vIntersection = vRayStartPos + fTime * vRayDir;

  return true; 
}

bool ezPlane::GetRayIntersectionBiDirectional(const ezVec3& vRayStartPos, const ezVec3& vRayDir, float* out_fIntersection, ezVec3* out_vIntersection) const
{
  EZ_ASSERT(vRayStartPos.IsValid(), "Ray start position must be valid.");
  EZ_ASSERT(vRayDir.IsValid(), "Ray direction must be valid.");

  const float fPlaneSide = GetDistanceTo(vRayStartPos);
  const float fCosAlpha = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0.0f)  // ray is orthogonal to plane
    return false;

  const float fTime = -fPlaneSide / fCosAlpha;

  if (out_fIntersection)
    *out_fIntersection = fTime;

  if (out_vIntersection)
    *out_vIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

bool ezPlane::GetLineSegmentIntersection(const ezVec3& vLineStartPos, const ezVec3& vLineEndPos, float* out_fHitFraction, ezVec3* out_vIntersection) const
{
  float fTime = 0.0f;

  if (!GetRayIntersection (vLineStartPos, vLineEndPos - vLineStartPos, &fTime, out_vIntersection))
    return false;

  if (out_fHitFraction)
    *out_fHitFraction = fTime;

  return (fTime <= 1.0f);
}

float ezPlane::GetMinimumDistanceTo(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof (ezVec3) */) const
{
  EZ_ASSERT(pPoints != NULL, "Array may not be NULL.");
  EZ_ASSERT(uiStride >= sizeof (ezVec3), "Stride must be at least sizeof(ezVec3) to not have overlapping data.");
  EZ_ASSERT(uiNumPoints >= 1, "Array must contain at least one point.");

  float fMinDist = ezMath::FloatMax_Pos();

  const ezVec3* pCurPoint = pPoints;

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    fMinDist = ezMath::Min (m_vNormal.Dot(*pCurPoint), fMinDist);

    pCurPoint = ezMemoryUtils::AddByteOffsetConst(pCurPoint, uiStride);
  }

  return fMinDist + m_fNegDistance;
}

void ezPlane::GetMinMaxDistanceTo(float &out_fMin, float &out_fMax, const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof (ezVec3) */) const
{
  EZ_ASSERT(pPoints != NULL, "Array may not be NULL.");
  EZ_ASSERT(uiStride >= sizeof (ezVec3), "Stride must be at least sizeof(ezVec3) to not have overlapping data.");
  EZ_ASSERT(uiNumPoints >= 1, "Array must contain at least one point.");

  out_fMin = ezMath::FloatMax_Pos();
  out_fMax = ezMath::FloatMax_Neg();

  const ezVec3* pCurPoint = pPoints;

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const float f = m_vNormal.Dot(*pCurPoint);

    out_fMin = ezMath::Min(f, out_fMin);
    out_fMax = ezMath::Max(f, out_fMax);

    pCurPoint = ezMemoryUtils::AddByteOffsetConst(pCurPoint, uiStride);
  }

  out_fMin += m_fNegDistance;
  out_fMax += m_fNegDistance;
}

ezResult ezPlane::GetPlanesIntersectionPoint(const ezPlane& p0, const ezPlane& p1, const ezPlane& p2, ezVec3& out_Result)
{
  const ezVec3 n1(p0.m_vNormal);
  const ezVec3 n2(p1.m_vNormal);
  const ezVec3 n3(p2.m_vNormal);

  const float det = n1.Dot(n2.Cross(n3));

  if (ezMath::IsZero(det))
    return EZ_FAILURE;

  out_Result = (-p0.m_fNegDistance * n2.Cross(n3) + 
                -p1.m_fNegDistance * n3.Cross(n1) + 
                -p2.m_fNegDistance * n1.Cross(n2)) / det;

  return EZ_SUCCESS;
}

