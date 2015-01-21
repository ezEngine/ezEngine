#pragma once

#include <Foundation/Math/Mat4.h>

template<typename Type>
EZ_FORCE_INLINE ezPlaneTemplate<Type>::ezPlaneTemplate()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  m_vNormal.Set(TypeNaN);
  m_fNegDistance = TypeNaN;
#endif
}

template<typename Type>
ezPlaneTemplate<Type>::ezPlaneTemplate(const ezVec3Template<Type>& vNormal, const ezVec3Template<Type>& vPointOnPlane)
{
  SetFromNormalAndPoint(vNormal, vPointOnPlane);
}

template<typename Type>
ezPlaneTemplate<Type>::ezPlaneTemplate(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3)
{
  SetFromPoints(v1, v2, v3);
}

template<typename Type>
ezPlaneTemplate<Type>::ezPlaneTemplate(const ezVec3Template<Type>* const pVertices)
{
  SetFromPoints(pVertices);
}

template<typename Type>
ezPlaneTemplate<Type>::ezPlaneTemplate(const ezVec3Template<Type>* const pVertices, ezUInt32 iMaxVertices)
{
  SetFromPoints(pVertices, iMaxVertices);
}

template<typename Type>
void ezPlaneTemplate<Type>::SetFromNormalAndPoint(const ezVec3Template<Type>& vNormal, const ezVec3Template<Type>& vPointOnPlane)
{
  EZ_ASSERT_DEBUG(vNormal.IsNormalized(), "Normal must be normalized.");

  m_vNormal = vNormal; 
  m_fNegDistance = -m_vNormal.Dot(vPointOnPlane);
}

template<typename Type>
ezResult ezPlaneTemplate<Type>::SetFromPoints(const ezVec3Template<Type>& v1, const ezVec3Template<Type>& v2, const ezVec3Template<Type>& v3)
{
  if (m_vNormal.CalculateNormal(v1, v2, v3) == EZ_FAILURE)
    return EZ_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(v1);
  return EZ_SUCCESS;
}

template<typename Type>
ezResult ezPlaneTemplate<Type>::SetFromPoints(const ezVec3Template<Type>* const pVertices)
{
  if (m_vNormal.CalculateNormal(pVertices[0], pVertices[1], pVertices[2]) == EZ_FAILURE)
    return EZ_FAILURE;

  m_fNegDistance = -m_vNormal.Dot(pVertices[0]);
  return EZ_SUCCESS;
}

template<typename Type>
ezResult ezPlaneTemplate<Type>::SetFromDirections(const ezVec3Template<Type>& vTangent1, const ezVec3Template<Type>& vTangent2, const ezVec3Template<Type>& vPointOnPlane)
{
  ezVec3Template<Type> vNormal = vTangent1.Cross(vTangent2);
  ezResult res = vNormal.NormalizeIfNotZero();

  m_vNormal  =  vNormal;
  m_fNegDistance = -vNormal.Dot(vPointOnPlane);
  return res;
}

template<typename Type>
void ezPlaneTemplate<Type>::Transform(const ezMat3Template<Type>& m)
{
  ezVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // rotate the normal, translate the point
  SetFromNormalAndPoint(m.TransformDirection(m_vNormal), m * vPointOnPlane);
}

template<typename Type>
void ezPlaneTemplate<Type>::Transform(const ezMat4Template<Type>& m)
{
  ezVec3Template<Type> vPointOnPlane = m_vNormal * -m_fNegDistance;

  // rotate the normal, translate the point
  SetFromNormalAndPoint(m.TransformDirection(m_vNormal), m * vPointOnPlane);
}

template<typename Type>
EZ_FORCE_INLINE void ezPlaneTemplate<Type>::Flip()
{
  m_fNegDistance = -m_fNegDistance;
  m_vNormal = -m_vNormal;
}

template<typename Type>
EZ_FORCE_INLINE Type ezPlaneTemplate<Type>::GetDistanceTo(const ezVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot (vPoint) + m_fNegDistance);
}

template<typename Type>
EZ_FORCE_INLINE ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetPointPosition(const ezVec3Template<Type>& vPoint) const
{
  return (m_vNormal.Dot(vPoint) < -m_fNegDistance ? ezPositionOnPlane::Back : ezPositionOnPlane::Front);
}

template<typename Type>
ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetPointPosition(const ezVec3Template<Type>& vPoint, Type fPlaneHalfWidth) const
{
  const Type f = m_vNormal.Dot(vPoint);

  if (f + fPlaneHalfWidth < -m_fNegDistance)
    return ezPositionOnPlane::Back;

  if (f - fPlaneHalfWidth > -m_fNegDistance)
    return ezPositionOnPlane::Front;

  return ezPositionOnPlane::OnPlane;
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezPlaneTemplate<Type>::ProjectOntoPlane(const ezVec3Template<Type>& vPoint) const
{
  return vPoint - m_vNormal * (m_vNormal.Dot(vPoint) + m_fNegDistance);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezPlaneTemplate<Type>::Mirror(const ezVec3Template<Type>& vPoint) const
{
  return vPoint - (Type) 2 * GetDistanceTo(vPoint) * m_vNormal;
}

template<typename Type>
const ezVec3Template<Type> ezPlaneTemplate<Type>::GetCoplanarDirection(const ezVec3Template<Type>& vDirection) const
{
  ezVec3Template<Type> res = vDirection;
  res.MakeOrthogonalTo(m_vNormal);
  return res;
}

template<typename Type>
bool ezPlaneTemplate<Type>::IsIdentical(const ezPlaneTemplate& rhs) const
{
  return m_vNormal.IsIdentical(rhs.m_vNormal) && m_fNegDistance == rhs.m_fNegDistance;
}

template<typename Type>
bool ezPlaneTemplate<Type>::IsEqual(const ezPlaneTemplate& rhs, Type fEpsilon) const
{
  return m_vNormal.IsEqual(rhs.m_vNormal, fEpsilon) && ezMath::IsEqual(m_fNegDistance, rhs.m_fNegDistance, fEpsilon);
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezPlaneTemplate<Type>& lhs, const ezPlaneTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezPlaneTemplate<Type>& lhs, const ezPlaneTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template<typename Type>
bool ezPlaneTemplate<Type>::FlipIfNecessary(const ezVec3Template<Type>& vPoint, bool bPlaneShouldFacePoint)
{
  if ((GetPointPosition(vPoint) == ezPositionOnPlane::Front) != bPlaneShouldFacePoint)
  {
    Flip();
    return true;
  }

  return false;
}

template<typename Type>
void ezPlaneTemplate<Type>::SetInvalid()
{
  m_vNormal.Set(0);
  m_fNegDistance = 0;
}

template<typename Type>
bool ezPlaneTemplate<Type>::IsValid() const
{
  return ezMath::IsFinite(m_fNegDistance) && m_vNormal.IsNormalized(ezMath::BasicType<Type>::DefaultEpsilon());
}

template<typename Type>
bool ezPlaneTemplate<Type>::IsNaN() const
{
  return ezMath::IsNaN(m_fNegDistance) || m_vNormal.IsNaN();
}


/*! The given vertices can be partially equal or lie on the same line. The algorithm will try to find 3 vertices, that
  form a plane, and deduce the normal from them. This algorithm is much slower, than all the other methods, so only
  use it, when you know, that your data can contain such configurations. */
template<typename Type>
ezResult ezPlaneTemplate<Type>::SetFromPoints(const ezVec3Template<Type>* const pVertices, ezUInt32 iMaxVertices)
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

template<typename Type>
ezResult ezPlaneTemplate<Type>::FindSupportPoints (const ezVec3Template<Type>* const pVertices, int iMaxVertices, int& out_v1, int& out_v2, int& out_v3)
{
  const ezVec3Template<Type> v1 = pVertices[0];

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

  const ezVec3Template<Type> v2 = pVertices[i];

  const ezVec3Template<Type> vDir1 = (v1 - v2).GetNormalized();

  out_v1 = 0;
  out_v2 = i;

  ++i;

  while (i < iMaxVertices)
  {
    // check for inequality, then for non-collinearity
    if ((pVertices[i].IsEqual (v2, 0.001f) == false) && 
      (ezMath::Abs ((pVertices[i] - v2).GetNormalized().Dot (vDir1)) < (Type) 0.999))
    {
      out_v3 = i;
      return EZ_SUCCESS;
    }

    ++i;
  }
    
  return EZ_FAILURE;
}

template<typename Type>
ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetObjectPosition (const ezVec3Template<Type>* const vPoints, ezUInt32 iVertices) const
{
  bool bFront = false;
  bool bBack = false;

  for (ezUInt32 i = 0; i < iVertices; ++i)
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
        
    default:
      break;
    }
  }

  return (bFront ? ezPositionOnPlane::Front : ezPositionOnPlane::Back);
}

template<typename Type>
ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetObjectPosition (const ezVec3Template<Type>* const vPoints, ezUInt32 iVertices, Type fPlaneHalfWidth) const
{
  bool bFront = false;
  bool bBack = false;

  for (ezUInt32 i = 0; i < iVertices; ++i)
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
        
    default:
      break;
    }
  }

  if (bFront)
    return (ezPositionOnPlane::Front);
  if (bBack)
    return (ezPositionOnPlane::Back);

  return (ezPositionOnPlane::OnPlane);
}

template<typename Type>
bool ezPlaneTemplate<Type>::GetRayIntersection(const ezVec3Template<Type>& vRayStartPos, const ezVec3Template<Type>& vRayDir, Type* out_fIntersection, ezVec3Template<Type>* out_vIntersection) const
{
  EZ_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  EZ_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0)  // ray is orthogonal to plane
    return false;

  if (ezMath::Sign(fPlaneSide) == ezMath::Sign(fCosAlpha)) // ray points away from the plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_fIntersection)
    *out_fIntersection = fTime;

  if (out_vIntersection)
    *out_vIntersection = vRayStartPos + fTime * vRayDir;

  return true; 
}

template<typename Type>
bool ezPlaneTemplate<Type>::GetRayIntersectionBiDirectional(const ezVec3Template<Type>& vRayStartPos, const ezVec3Template<Type>& vRayDir, Type* out_fIntersection, ezVec3Template<Type>* out_vIntersection) const
{
  EZ_ASSERT_DEBUG(vRayStartPos.IsValid(), "Ray start position must be valid.");
  EZ_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  const Type fPlaneSide = GetDistanceTo(vRayStartPos);
  const Type fCosAlpha = m_vNormal.Dot(vRayDir);

  if (fCosAlpha == 0)  // ray is orthogonal to plane
    return false;

  const Type fTime = -fPlaneSide / fCosAlpha;

  if (out_fIntersection)
    *out_fIntersection = fTime;

  if (out_vIntersection)
    *out_vIntersection = vRayStartPos + fTime * vRayDir;

  return true;
}

template<typename Type>
bool ezPlaneTemplate<Type>::GetLineSegmentIntersection(const ezVec3Template<Type>& vLineStartPos, const ezVec3Template<Type>& vLineEndPos, Type* out_fHitFraction, ezVec3Template<Type>* out_vIntersection) const
{
  Type fTime = 0;

  if (!GetRayIntersection (vLineStartPos, vLineEndPos - vLineStartPos, &fTime, out_vIntersection))
    return false;

  if (out_fHitFraction)
    *out_fHitFraction = fTime;

  return (fTime <= 1);
}

template<typename Type>
Type ezPlaneTemplate<Type>::GetMinimumDistanceTo(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof (ezVec3Template<Type>) */) const
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof (ezVec3Template<Type>), "Stride must be at least sizeof(ezVec3Template) to not have overlapping data.");
  EZ_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  Type fMinDist = ezMath::BasicType<Type>::MaxValue();

  const ezVec3Template<Type>* pCurPoint = pPoints;

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    fMinDist = ezMath::Min (m_vNormal.Dot(*pCurPoint), fMinDist);

    pCurPoint = ezMemoryUtils::AddByteOffsetConst(pCurPoint, uiStride);
  }

  return fMinDist + m_fNegDistance;
}

template<typename Type>
void ezPlaneTemplate<Type>::GetMinMaxDistanceTo(Type &out_fMin, Type &out_fMax, const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof (ezVec3Template<Type>) */) const
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof (ezVec3Template<Type>), "Stride must be at least sizeof(ezVec3Template) to not have overlapping data.");
  EZ_ASSERT_DEBUG(uiNumPoints >= 1, "Array must contain at least one point.");

  out_fMin =  ezMath::BasicType<Type>::MaxValue();
  out_fMax = -ezMath::BasicType<Type>::MaxValue();

  const ezVec3Template<Type>* pCurPoint = pPoints;

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type f = m_vNormal.Dot(*pCurPoint);

    out_fMin = ezMath::Min(f, out_fMin);
    out_fMax = ezMath::Max(f, out_fMax);

    pCurPoint = ezMemoryUtils::AddByteOffsetConst(pCurPoint, uiStride);
  }

  out_fMin += m_fNegDistance;
  out_fMax += m_fNegDistance;
}

template<typename Type>
ezResult ezPlaneTemplate<Type>::GetPlanesIntersectionPoint(const ezPlaneTemplate& p0, const ezPlaneTemplate& p1, const ezPlaneTemplate& p2, ezVec3Template<Type>& out_Result)
{
  const ezVec3Template<Type> n1(p0.m_vNormal);
  const ezVec3Template<Type> n2(p1.m_vNormal);
  const ezVec3Template<Type> n3(p2.m_vNormal);

  const Type det = n1.Dot(n2.Cross(n3));

  if (ezMath::IsZero<Type>(det, ezMath::BasicType<Type>::LargeEpsilon()))
    return EZ_FAILURE;

  out_Result = (-p0.m_fNegDistance * n2.Cross(n3) + 
                -p1.m_fNegDistance * n3.Cross(n1) + 
                -p2.m_fNegDistance * n1.Cross(n2)) / det;

  return EZ_SUCCESS;
}

#include <Foundation/Math/Implementation/AllClasses_inl.h>

