#pragma once

#include <Foundation/Math/Mat4.h>

template<typename Type>
EZ_FORCE_INLINE ezBoundingBoxTemplate<Type>::ezBoundingBoxTemplate()
{
}

template<typename Type>
EZ_FORCE_INLINE ezBoundingBoxTemplate<Type>::ezBoundingBoxTemplate(const ezVec3Template<Type>& vMin, const ezVec3Template<Type>& vMax)
{
  SetElements(vMin, vMax);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingBoxTemplate<Type>::SetElements(const ezVec3Template<Type>& vMin, const ezVec3Template<Type>& vMax)
{
  m_vMin = vMin;
  m_vMax = vMax;

  EZ_ASSERT_DEBUG(IsValid(), "The given values did not create a valid bounding box (%.2f | %.2f | %.2f - %.2f | %.2f | %.2f)", vMin.x, vMin.y, vMin.z, vMax.x, vMax.y, vMax.z);
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::SetFromPoints(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template<Type>) */)
{
  SetInvalid();
  ExpandToInclude(pPoints, uiNumPoints, uiStride);
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::GetCorners(ezVec3Template<Type>* out_pCorners) const
{
  EZ_NAN_ASSERT(this);
  EZ_ASSERT_DEBUG(out_pCorners != nullptr, "Out Parameter must not be nullptr.");

  out_pCorners[0].Set(m_vMin.x, m_vMin.y, m_vMin.z);
  out_pCorners[1].Set(m_vMin.x, m_vMin.y, m_vMax.z);
  out_pCorners[2].Set(m_vMin.x, m_vMax.y, m_vMin.z);
  out_pCorners[3].Set(m_vMin.x, m_vMax.y, m_vMax.z);
  out_pCorners[4].Set(m_vMax.x, m_vMin.y, m_vMin.z);
  out_pCorners[5].Set(m_vMax.x, m_vMin.y, m_vMax.z);
  out_pCorners[6].Set(m_vMax.x, m_vMax.y, m_vMin.z);
  out_pCorners[7].Set(m_vMax.x, m_vMax.y, m_vMax.z);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezBoundingBoxTemplate<Type>::GetCenter() const
{
  return m_vMin + GetHalfExtents();
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezBoundingBoxTemplate<Type>::GetExtents() const
{
  return m_vMax - m_vMin;
}

template<typename Type>
const ezVec3Template<Type> ezBoundingBoxTemplate<Type>::GetHalfExtents() const
{
  return (m_vMax - m_vMin) / (Type) 2;
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::SetCenterAndHalfExtents(const ezVec3Template<Type>& vCenter, const ezVec3Template<Type>& vHalfExtents)
{
  m_vMin = vCenter - vHalfExtents;
  m_vMax = vCenter + vHalfExtents;
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::SetInvalid()
{
  m_vMin.Set(ezMath::BasicType<Type>::MaxValue());
  m_vMax.Set(-ezMath::BasicType<Type>::MaxValue());
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::IsValid() const
{
  return (m_vMin.IsValid() && m_vMax.IsValid() && m_vMin.x <= m_vMax.x && m_vMin.y <= m_vMax.y && m_vMin.z <= m_vMax.z);
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::IsNaN() const
{
  return m_vMin.IsNaN() || m_vMax.IsNaN();
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingBoxTemplate<Type>::ExpandToInclude(const ezVec3Template<Type>& vPoint)
{
  m_vMin = m_vMin.CompMin(vPoint);
  m_vMax = m_vMax.CompMax(vPoint);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingBoxTemplate<Type>::ExpandToInclude(const ezBoundingBoxTemplate<Type>& rhs)
{
  ExpandToInclude(rhs.m_vMin);
  ExpandToInclude(rhs.m_vMax);
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::ExpandToInclude(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "Array may not be nullptr.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "Data may not overlap.");

  const ezVec3Template<Type>* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::ExpandToCube()
{
  ezVec3Template<Type> vHalfExtents = GetHalfExtents();
  const ezVec3Template<Type> vCenter = m_vMin + vHalfExtents;

  const Type f = ezMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);

  m_vMin = vCenter - ezVec3Template<Type>(f);
  m_vMax = vCenter + ezVec3Template<Type>(f);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingBoxTemplate<Type>::Grow(const ezVec3Template<Type>& vDiff)
{
  EZ_ASSERT_DEBUG(IsValid(), "Cannot grow a box that is invalid.");

  m_vMax += vDiff;
  m_vMin -= vDiff;

  EZ_ASSERT_DEBUG(IsValid(), "The grown box has become invalid.");
}

template<typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxTemplate<Type>::Contains(const ezVec3Template<Type>& vPoint) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&vPoint);

  return(ezMath::IsInRange(vPoint.x, m_vMin.x, m_vMax.x) &&
         ezMath::IsInRange(vPoint.y, m_vMin.y, m_vMax.y) &&
         ezMath::IsInRange(vPoint.z, m_vMin.z, m_vMax.z));
}

template<typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxTemplate<Type>::Contains(const ezBoundingBoxTemplate<Type>& rhs) const
{
  return Contains(rhs.m_vMin) && Contains(rhs.m_vMax);
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::Contains(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template<Type>) */) const
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "Data must not overlap.");

  const ezVec3Template<Type>* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (!Contains(*pCur))
      return false;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return true;
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::Overlaps(const ezBoundingBoxTemplate<Type>& rhs) const
{
  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  if (rhs.m_vMin.x >= m_vMax.x)
    return false;
  if (rhs.m_vMin.y >= m_vMax.y)
    return false;
  if (rhs.m_vMin.z >= m_vMax.z)
    return false;

  if (m_vMin.x >= rhs.m_vMax.x)
    return false;
  if (m_vMin.y >= rhs.m_vMax.y)
    return false;
  if (m_vMin.z >= rhs.m_vMax.z)
    return false;

  return true;
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::Overlaps(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template<Type>) */) const
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "Array must not be NuLL.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "Data must not overlap.");

  const ezVec3Template<Type>* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (Contains(*pCur))
      return true;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return false;
}

template<typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxTemplate<Type>::IsIdentical(const ezBoundingBoxTemplate<Type>& rhs) const
{
  return (m_vMin == rhs.m_vMin && m_vMax == rhs.m_vMax);
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::IsEqual(const ezBoundingBoxTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vMin.IsEqual(rhs.m_vMin, fEpsilon) && m_vMax.IsEqual(rhs.m_vMax, fEpsilon));
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezBoundingBoxTemplate<Type>& lhs, const ezBoundingBoxTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezBoundingBoxTemplate<Type>& lhs, const ezBoundingBoxTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingBoxTemplate<Type>::Translate(const ezVec3Template<Type>& vDiff)
{
  m_vMin += vDiff;
  m_vMax += vDiff;
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::ScaleFromCenter(const ezVec3Template<Type>& vScale)
{
  const ezVec3Template<Type> vCenter = GetCenter();
  m_vMin = vCenter + (m_vMin - vCenter).CompMult(vScale);
  m_vMax = vCenter + (m_vMax - vCenter).CompMult(vScale);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingBoxTemplate<Type>::ScaleFromOrigin(const ezVec3Template<Type>& vScale)
{
  m_vMin = m_vMin.CompMult(vScale);
  m_vMax = m_vMax.CompMult(vScale);
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::TransformFromCenter (const ezMat4Template<Type>& mTransform)
{
  ezVec3Template<Type> vCorners[8];
  GetCorners (vCorners);

  const ezVec3Template<Type> vCenter = GetCenter ();
  SetInvalid ();

  for (ezUInt32 i = 0; i < 8; ++i)
    ExpandToInclude (vCenter + mTransform.TransformPosition (vCorners[i] - vCenter));
}

template<typename Type>
void ezBoundingBoxTemplate<Type>::TransformFromOrigin(const ezMat4Template<Type>& mTransform)
{
  ezVec3Template<Type> vCorners[8];
  GetCorners(vCorners);

  mTransform.TransformPosition(vCorners, 8);

  SetInvalid();
  ExpandToInclude(vCorners, 8);
}

template<typename Type>
EZ_FORCE_INLINE const ezVec3Template<Type> ezBoundingBoxTemplate<Type>::GetClampedPoint(const ezVec3Template<Type>& vPoint) const
{
  return vPoint.CompMin(m_vMax).CompMax(m_vMin);
}

template<typename Type>
Type ezBoundingBoxTemplate<Type>::GetDistanceTo(const ezVec3Template<Type>& vPoint) const
{
  const ezVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength();
}

template<typename Type>
Type ezBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const ezVec3Template<Type>& vPoint) const
{
  const ezVec3Template<Type> vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared();
}

template<typename Type>
Type ezBoundingBoxTemplate<Type>::GetDistanceSquaredTo(const ezBoundingBoxTemplate<Type>& rhs) const
{
  // This will return zero for overlapping boxes

  EZ_NAN_ASSERT(this);
  EZ_NAN_ASSERT(&rhs);

  Type fDistSQR = 0.0f;

  {
    if (rhs.m_vMin.x > m_vMax.x)
    {
      fDistSQR += ezMath::Square(rhs.m_vMin.x - m_vMax.x);
    }
    else
    if (rhs.m_vMax.x < m_vMin.x)
    {
      fDistSQR += ezMath::Square(m_vMin.x - rhs.m_vMax.x);
    }
  }

  {
    if (rhs.m_vMin.y > m_vMax.y)
    {
      fDistSQR += ezMath::Square(rhs.m_vMin.y - m_vMax.y);
    }
    else
    if (rhs.m_vMax.y < m_vMin.y)
    {
      fDistSQR += ezMath::Square(m_vMin.y - rhs.m_vMax.y);
    }
  }

  {
    if (rhs.m_vMin.z > m_vMax.z)
    {
      fDistSQR += ezMath::Square(rhs.m_vMin.z - m_vMax.z);
    }
    else
    if (rhs.m_vMax.z < m_vMin.z)
    {
      fDistSQR += ezMath::Square(m_vMin.z - rhs.m_vMax.z);
    }
  }

  return fDistSQR;
}

template<typename Type>
Type ezBoundingBoxTemplate<Type>::GetDistanceTo(const ezBoundingBoxTemplate<Type>& rhs) const
{
  return ezMath::Sqrt(GetDistanceSquaredTo(rhs));
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::GetRayIntersection(const ezVec3Template<Type>& vStartPos, const ezVec3Template<Type>& vRayDir, Type* out_fIntersection, ezVec3Template<Type>* out_vIntersection) const
{
  // This code was taken from: http://people.csail.mit.edu/amy/papers/box-jgt.pdf
  // "An Efficient and Robust Ray-Box Intersection Algorithm"
  // Contrary to previous implementation, this one actually works with ray/box configurations
  // that produce division by zero and multiplication with infinity (which can produce NaNs).

  EZ_ASSERT_DEBUG(ezMath::BasicType<Type>::SupportsInfinity(), "This type does not support infinite values, which is required for this algorithm.");
  EZ_ASSERT_DEBUG(vStartPos.IsValid(), "Ray start position must be valid.");
  EZ_ASSERT_DEBUG(vRayDir.IsValid(), "Ray direction must be valid.");

  EZ_NAN_ASSERT(this);

  float tMin, tMax;

  // Compare along X and Z axis, find intersection point
  {
    float tMinY, tMaxY;

    const float fDivX = 1.0f / vRayDir.x;
    const float fDivY = 1.0f / vRayDir.y;

    if (vRayDir.x >= 0.0f)
    {
      tMin = (m_vMin.x - vStartPos.x) * fDivX;
      tMax = (m_vMax.x - vStartPos.x) * fDivX;
    }
    else
    {
      tMin = (m_vMax.x - vStartPos.x) * fDivX;
      tMax = (m_vMin.x - vStartPos.x) * fDivX;
    }

    if (vRayDir.y >= 0.0f)
    {
      tMinY = (m_vMin.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMax.y - vStartPos.y) * fDivY;
    }
    else
    {
      tMinY = (m_vMax.y - vStartPos.y) * fDivY;
      tMaxY = (m_vMin.y - vStartPos.y) * fDivY;
    }

    if (tMin > tMaxY || tMinY > tMax)
      return false;

    if (tMinY > tMin) tMin = tMinY;
    if (tMaxY < tMax) tMax = tMaxY;
  }

  // Compare along Z axis and previous result, find intersection point
  {
    float tMinZ, tMaxZ;

    const float fDivZ = 1.0f / vRayDir.z;

    if (vRayDir.z >= 0.0f)
    {
      tMinZ = (m_vMin.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMax.z - vStartPos.z) * fDivZ;
    }
    else
    {
      tMinZ = (m_vMax.z - vStartPos.z) * fDivZ;
      tMaxZ = (m_vMin.z - vStartPos.z) * fDivZ;
    }

    if (tMin > tMaxZ || tMinZ > tMax)
      return false;

    if (tMinZ > tMin) tMin = tMinZ;
    if (tMaxZ < tMax) tMax = tMaxZ;
  }

  // rays that start inside the box are considered as not hitting the box
  if (tMax <= 0.0f)
    return false;

  if (out_fIntersection)
    *out_fIntersection = tMin;

  if (out_vIntersection)
    *out_vIntersection = vStartPos + tMin * vRayDir;

  return true;
}

template<typename Type>
bool ezBoundingBoxTemplate<Type>::GetLineSegmentIntersection(const ezVec3Template<Type>& vStartPos, const ezVec3Template<Type>& vEndPos, Type* out_fLineFraction, ezVec3Template<Type>* out_vIntersection) const
{
  const ezVec3Template<Type> vRayDir = vEndPos - vStartPos;

  Type fIntersection = 0.0f;
  if (!GetRayIntersection(vStartPos, vRayDir, &fIntersection, out_vIntersection))
    return false;

  if (out_fLineFraction)
    *out_fLineFraction = fIntersection;

  return fIntersection <= 1.0f;
}



#include <Foundation/Math/Implementation/AllClasses_inl.h>

