#pragma once

#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/BoundingSphere.h>

EZ_FORCE_INLINE ezBoundingBox::ezBoundingBox()
{
}

EZ_FORCE_INLINE ezBoundingBox::ezBoundingBox(const ezVec3& vMin, const ezVec3& vMax)
{
  SetElements(vMin, vMax);
}

EZ_FORCE_INLINE void ezBoundingBox::SetElements(const ezVec3& vMin, const ezVec3& vMax)
{
  m_vMin = vMin;
  m_vMax = vMax;

  EZ_ASSERT(IsValid(), "The given values did not create a valid bounding box (%.2f | %.2f | %.2f - %.2f | %.2f | %.2f)", vMin.x, vMin.y, vMin.z, vMax.x, vMax.y, vMax.z);
}

inline void ezBoundingBox::SetFromPoints(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */)
{
  SetInvalid();
  ExpandToInclude(pPoints, uiNumPoints, uiStride);
}

inline void ezBoundingBox::GetCorners(ezVec3* out_pCorners) const
{
  EZ_ASSERT(out_pCorners != NULL, "Out Parameter must not be NULL.");

  out_pCorners[0].Set(m_vMin.x, m_vMin.y, m_vMin.z);
  out_pCorners[1].Set(m_vMin.x, m_vMin.y, m_vMax.z);
  out_pCorners[2].Set(m_vMin.x, m_vMax.y, m_vMin.z);
  out_pCorners[3].Set(m_vMin.x, m_vMax.y, m_vMax.z);
  out_pCorners[4].Set(m_vMax.x, m_vMin.y, m_vMin.z);
  out_pCorners[5].Set(m_vMax.x, m_vMin.y, m_vMax.z);
  out_pCorners[6].Set(m_vMax.x, m_vMax.y, m_vMin.z);
  out_pCorners[7].Set(m_vMax.x, m_vMax.y, m_vMax.z);
}

EZ_FORCE_INLINE const ezVec3 ezBoundingBox::GetCenter() const
{
  return m_vMin + GetHalfExtents();
}

EZ_FORCE_INLINE const ezVec3 ezBoundingBox::GetExtents() const
{
  return m_vMax - m_vMin;
}

inline const ezVec3 ezBoundingBox::GetHalfExtents() const
{
  return (m_vMax - m_vMin) * 0.5f;
}

inline void ezBoundingBox::SetCenterAndHalfExtents(const ezVec3& vCenter, const ezVec3& vHalfExtents)
{
  m_vMin = vCenter - vHalfExtents;
  m_vMax = vCenter + vHalfExtents;
}

inline void ezBoundingBox::SetInvalid()
{
  m_vMin.Set(ezMath::FloatMax_Pos());
  m_vMax.Set(ezMath::FloatMax_Neg());
}

inline bool ezBoundingBox::IsValid() const
{
  return (m_vMin.IsValid() && m_vMax.IsValid() && m_vMin.x <= m_vMax.x && m_vMin.y <= m_vMax.y && m_vMin.z <= m_vMax.z);
}

EZ_FORCE_INLINE void ezBoundingBox::ExpandToInclude(const ezVec3& vPoint)
{
  m_vMin = m_vMin.CompMin(vPoint);
  m_vMax = m_vMax.CompMax(vPoint);
}

EZ_FORCE_INLINE void ezBoundingBox::ExpandToInclude(const ezBoundingBox& rhs)
{
  ExpandToInclude(rhs.m_vMin);
  ExpandToInclude(rhs.m_vMax);
}

inline void ezBoundingBox::ExpandToInclude(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride)
{
  EZ_ASSERT(pPoints != NULL, "Array may not be NULL.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "Data may not overlap.");

  const ezVec3* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    ExpandToInclude(*pCur);

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }
}

inline void ezBoundingBox::ExpandToCube()
{
  ezVec3 vHalfExtents = GetHalfExtents();
  const ezVec3 vCenter = m_vMin + vHalfExtents;

  const float f = ezMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);

  m_vMin = vCenter - ezVec3(f);
  m_vMax = vCenter + ezVec3(f);
}

EZ_FORCE_INLINE void ezBoundingBox::Grow(const ezVec3& vDiff)
{
  EZ_ASSERT(IsValid(), "Cannot grow a box that is invalid.");

  m_vMax += vDiff;
  m_vMin -= vDiff;

  EZ_ASSERT(IsValid(), "The grown box has become invalid.");
}

EZ_FORCE_INLINE bool ezBoundingBox::Contains(const ezVec3& vPoint) const
{
  return(ezMath::IsInRange(vPoint.x, m_vMin.x, m_vMax.x) &&
         ezMath::IsInRange(vPoint.y, m_vMin.y, m_vMax.y) &&
         ezMath::IsInRange(vPoint.z, m_vMin.z, m_vMax.z));
}

EZ_FORCE_INLINE bool ezBoundingBox::Contains(const ezBoundingBox& rhs) const
{
  return Contains(rhs.m_vMin) && Contains(rhs.m_vMax);
}

inline bool ezBoundingBox::Contains(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */) const
{
  EZ_ASSERT(pPoints != NULL, "Array must not be NuLL.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "Data must not overlap.");

  const ezVec3* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (!Contains(*pCur))
      return false;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return true;
}

EZ_FORCE_INLINE bool ezBoundingBox::Contains(const ezBoundingSphere& sphere) const
{
  return Contains(sphere.GetBoundingBox());
}

inline bool ezBoundingBox::Overlaps(const ezBoundingBox& rhs) const
{
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

inline bool ezBoundingBox::Overlaps(const ezVec3* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3) */) const
{
  EZ_ASSERT(pPoints != NULL, "Array must not be NuLL.");
  EZ_ASSERT(uiStride >= sizeof(ezVec3), "Data must not overlap.");

  const ezVec3* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if (Contains(*pCur))
      return true;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return false;
}

EZ_FORCE_INLINE bool ezBoundingBox::Overlaps(const ezBoundingSphere& sphere) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return sphere.Contains(GetClampedPoint(sphere.m_vCenter));
}

EZ_FORCE_INLINE bool ezBoundingBox::IsIdentical(const ezBoundingBox& rhs) const
{
  return (m_vMin == rhs.m_vMin && m_vMax == rhs.m_vMax);
}

inline bool ezBoundingBox::IsEqual(const ezBoundingBox& rhs, float fEpsilon) const
{
  return (m_vMin.IsEqual(rhs.m_vMin, fEpsilon) && m_vMax.IsEqual(rhs.m_vMax, fEpsilon));
}

EZ_FORCE_INLINE bool operator== (const ezBoundingBox& lhs, const ezBoundingBox& rhs)
{
  return lhs.IsIdentical(rhs);
}

EZ_FORCE_INLINE bool operator!= (const ezBoundingBox& lhs, const ezBoundingBox& rhs)
{
  return !lhs.IsIdentical(rhs);
}

EZ_FORCE_INLINE void ezBoundingBox::Translate(const ezVec3& vDiff)
{
  m_vMin += vDiff;
  m_vMax += vDiff;
}

inline void ezBoundingBox::ScaleFromCenter(const ezVec3& vScale)
{
  const ezVec3 vCenter = GetCenter();
  m_vMin = vCenter + (m_vMin - vCenter).CompMult(vScale);
  m_vMax = vCenter + (m_vMax - vCenter).CompMult(vScale);
}

EZ_FORCE_INLINE void ezBoundingBox::ScaleFromOrigin(const ezVec3& vScale)
{
  m_vMin = m_vMin.CompMult(vScale);
  m_vMax = m_vMax.CompMult(vScale);
}

inline void ezBoundingBox::TransformFromCenter (const ezMat4& mTransform)
{
  ezVec3 vCorners[8];
  GetCorners (vCorners);

  const ezVec3 vCenter = GetCenter ();
  SetInvalid ();

  for (ezUInt32 i = 0; i < 8; ++i)
    ExpandToInclude (vCenter + mTransform.TransformPosition (vCorners[i] - vCenter));
}

inline void ezBoundingBox::TransformFromOrigin(const ezMat4& mTransform)
{
  ezVec3 vCorners[8];
  GetCorners(vCorners);

  mTransform.TransformPosition(vCorners, 8);

  SetInvalid();
  ExpandToInclude(vCorners, 8);
}

EZ_FORCE_INLINE const ezVec3 ezBoundingBox::GetClampedPoint(const ezVec3& vPoint) const
{
  return vPoint.CompMin(m_vMax).CompMax(m_vMin);
}

inline float ezBoundingBox::GetDistanceTo(const ezVec3& vPoint) const
{
  const ezVec3 vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLength();
}

inline float ezBoundingBox::GetDistanceTo(const ezBoundingSphere& sphere) const
{
  return (GetClampedPoint(sphere.m_vCenter) - sphere.m_vCenter).GetLength() - sphere.m_fRadius;
}

inline float ezBoundingBox::GetDistanceSquaredTo(const ezVec3& vPoint) const
{
  const ezVec3 vClamped = GetClampedPoint(vPoint);

  return (vPoint - vClamped).GetLengthSquared();
}

inline float ezBoundingBox::GetDistanceSquaredTo(const ezBoundingBox& rhs) const
{
  // This will return zero for overlapping boxes

  float fDistSQR = 0.0f;

  for (ezUInt32 i = 0; i < 3; ++i)
  {
    if (rhs.m_vMin.m_Data[i] > m_vMax.m_Data[i])
    {
      fDistSQR += ezMath::Square(rhs.m_vMin.m_Data[i] - m_vMax.m_Data[i]);
    }
    else
    if (rhs.m_vMax.m_Data[i] < m_vMin.m_Data[i])
    {
      fDistSQR += ezMath::Square(m_vMin.m_Data[i] - rhs.m_vMax.m_Data[i]);
    }
  }
  return fDistSQR;
}

inline float ezBoundingBox::GetDistanceTo(const ezBoundingBox& rhs) const
{
  return ezMath::Sqrt(GetDistanceSquaredTo(rhs));
}

inline const ezBoundingSphere ezBoundingBox::GetBoundingSphere() const
{
  return ezBoundingSphere(GetCenter(), (m_vMax - m_vMin).GetLength() * 0.5f);
}