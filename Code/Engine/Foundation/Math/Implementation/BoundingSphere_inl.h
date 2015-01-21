#pragma once

#include <Foundation/Math/Mat4.h>

template<typename Type>
EZ_FORCE_INLINE ezBoundingSphereTemplate<Type>::ezBoundingSphereTemplate()
{
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vCenter is already initialized to NaN by its own constructor.
  const Type TypeNaN = ezMath::BasicType<Type>::GetNaN();
  m_fRadius = TypeNaN;
#endif
}

template<typename Type>
EZ_FORCE_INLINE ezBoundingSphereTemplate<Type>::ezBoundingSphereTemplate(const ezVec3Template<Type>& vCenter, Type fRadius)
{
  m_vCenter = vCenter;
  m_fRadius = fRadius;
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::SetZero()
{
  m_vCenter.SetZero();
  m_fRadius = 0.0f;
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::IsZero(Type fEpsilon /* = ezMath::BasicType<Type>::DefaultEpsilon() */) const
{
  return m_vCenter.IsZero(fEpsilon) && ezMath::IsZero(m_fRadius, fEpsilon);
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::SetInvalid()
{
  m_vCenter.SetZero();
  m_fRadius = -1.0f;
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::IsValid() const
{
  return (m_vCenter.IsValid() && m_fRadius >= 0.0f);
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::IsNaN() const
{
  return (m_vCenter.IsNaN() || ezMath::IsNaN(m_fRadius));
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingSphereTemplate<Type>::SetElements(const ezVec3Template<Type>& vCenter, Type fRadius)
{
  m_vCenter = vCenter;
  m_fRadius = fRadius;

  EZ_ASSERT_DEBUG(IsValid(), "The sphere was created with invalid values.");
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::ExpandToInclude(const ezVec3Template<Type>& vPoint)
{
  const Type fDistSQR = (vPoint - m_vCenter).GetLengthSquared();

  if (ezMath::Square(m_fRadius) < fDistSQR)
    m_fRadius = ezMath::Sqrt(fDistSQR);
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::ExpandToInclude(const ezBoundingSphereTemplate<Type>& rhs)
{
  const Type fReqRadius = (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius;

  m_fRadius = ezMath::Max(m_fRadius, fReqRadius);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingSphereTemplate<Type>::Grow(Type fDiff)
{
  EZ_ASSERT_DEBUG(IsValid(), "Cannot grow a sphere that is invalid.");

  m_fRadius += fDiff;

  EZ_ASSERT_DEBUG(IsValid(), "The grown sphere has become invalid.");
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::IsIdentical(const ezBoundingSphereTemplate<Type>& rhs) const
{
  return (m_vCenter.IsIdentical(rhs.m_vCenter) && m_fRadius == rhs.m_fRadius);
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::IsEqual(const ezBoundingSphereTemplate<Type>& rhs, Type fEpsilon) const
{
  return (m_vCenter.IsEqual(rhs.m_vCenter, fEpsilon) && ezMath::IsEqual(m_fRadius, rhs.m_fRadius, fEpsilon));
}

template<typename Type>
EZ_FORCE_INLINE bool operator== (const ezBoundingSphereTemplate<Type>& lhs, const ezBoundingSphereTemplate<Type>& rhs)
{
  return lhs.IsIdentical(rhs);
}

template<typename Type>
EZ_FORCE_INLINE bool operator!= (const ezBoundingSphereTemplate<Type>& lhs, const ezBoundingSphereTemplate<Type>& rhs)
{
  return !lhs.IsIdentical(rhs);
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingSphereTemplate<Type>::Translate(const ezVec3Template<Type>& vTranslation)
{
  m_vCenter += vTranslation;
}

template<typename Type>
EZ_FORCE_INLINE void ezBoundingSphereTemplate<Type>::ScaleFromCenter(Type fScale)
{
  EZ_ASSERT_DEBUG(fScale >= 0.0f, "Cannot invert the sphere.");

  m_fRadius *= fScale;

  EZ_NAN_ASSERT(this);
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::ScaleFromOrigin(const ezVec3Template<Type>& vScale)
{
  EZ_ASSERT_DEBUG(vScale.x >= 0.0f, "Cannot invert the sphere.");
  EZ_ASSERT_DEBUG(vScale.y >= 0.0f, "Cannot invert the sphere.");
  EZ_ASSERT_DEBUG(vScale.z >= 0.0f, "Cannot invert the sphere.");

  m_vCenter = m_vCenter.CompMult(vScale);

  // scale the radius by the maximum scaling factor (the sphere cannot become an ellipsoid, 
  // so to be a 'bounding' sphere, it should be as large as possible
  m_fRadius *= ezMath::Max(vScale.x, vScale.y, vScale.z);
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::TransformFromOrigin (const ezMat4Template<Type>& mTransform)
{
  m_vCenter = mTransform.TransformPosition (m_vCenter);

  const ezVec3Template<Type> Scale = mTransform.GetScalingFactors ();
  m_fRadius *= ezMath::Max(Scale.x, Scale.y, Scale.z);
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::TransformFromCenter (const ezMat4Template<Type>& mTransform)
{
  m_vCenter += mTransform.GetTranslationVector();

  const ezVec3Template<Type> Scale = mTransform.GetScalingFactors ();
  m_fRadius *= ezMath::Max(Scale.x, Scale.y, Scale.z);
}

template<typename Type>
Type ezBoundingSphereTemplate<Type>::GetDistanceTo(const ezVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLength() - m_fRadius;
}

template<typename Type>
Type ezBoundingSphereTemplate<Type>::GetDistanceTo(const ezBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() - m_fRadius - rhs.m_fRadius;
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::Contains(const ezVec3Template<Type>& vPoint) const
{
  return (vPoint - m_vCenter).GetLengthSquared() <= ezMath::Square(m_fRadius);
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::Contains(const ezBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius <= m_fRadius;
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::Overlaps(const ezBoundingSphereTemplate<Type>& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLengthSquared() < ezMath::Square(rhs.m_fRadius + m_fRadius);
}

template<typename Type>
const ezVec3Template<Type> ezBoundingSphereTemplate<Type>::GetClampedPoint(const ezVec3Template<Type>& vPoint)
{
  const ezVec3Template<Type> vDir = vPoint - m_vCenter;
  const Type fDistSQR = vDir.GetLengthSquared ();
  
  // return the point, if it is already inside the sphere
  if (fDistSQR <= ezMath::Square (m_fRadius))
    return vPoint;

  // otherwise return a point on the surface of the sphere

  const Type fLength = ezMath::Sqrt (fDistSQR);

  return m_vCenter + m_fRadius * (vDir / fLength);
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::Contains(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template) */) const
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = ezMath::Square(m_fRadius);

  const ezVec3Template<Type>* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() > fRadiusSQR)
      return false;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return true;
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::Overlaps(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template) */) const
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "The data must not overlap.");

  const Type fRadiusSQR = ezMath::Square(m_fRadius);

  const ezVec3Template<Type>* pCur = &pPoints[0];

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    if ((*pCur - m_vCenter).GetLengthSquared() <= fRadiusSQR)
      return true;

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return false;
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::SetFromPoints(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template) */)
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "The data must not overlap.");
  EZ_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");

  const ezVec3Template<Type>* pCur = &pPoints[0];

  ezVec3Template<Type> vCenter(0.0f);

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    vCenter += *pCur;
    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  vCenter /= (Type) uiNumPoints;

  Type fMaxDistSQR = 0.0f;

  pCur = &pPoints[0];
  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    fMaxDistSQR = (*pCur - vCenter).GetLengthSquared();
    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  m_vCenter = vCenter;
  m_fRadius = ezMath::Sqrt(fMaxDistSQR);

  EZ_ASSERT_DEBUG(IsValid(), "The point cloud contained corrupted data.");
}

template<typename Type>
void ezBoundingSphereTemplate<Type>::ExpandToInclude(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template) */)
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "The data must not overlap.");

  const ezVec3Template<Type>* pCur = &pPoints[0];

  Type fMaxDistSQR = 0.0f;

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();

    fMaxDistSQR = ezMath::Max(fMaxDistSQR, fDistSQR);

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  if (ezMath::Square(m_fRadius) < fMaxDistSQR)
    m_fRadius = ezMath::Sqrt(fMaxDistSQR);
}

template<typename Type>
Type ezBoundingSphereTemplate<Type>::GetDistanceTo(const ezVec3Template<Type>* pPoints, ezUInt32 uiNumPoints, ezUInt32 uiStride /* = sizeof(ezVec3Template) */) const
{
  EZ_ASSERT_DEBUG(pPoints != nullptr, "The array must not be empty.");
  EZ_ASSERT_DEBUG(uiNumPoints > 0, "The array must contain at least one point.");
  EZ_ASSERT_DEBUG(uiStride >= sizeof(ezVec3Template<Type>), "The data must not overlap.");

  const ezVec3Template<Type>* pCur = &pPoints[0];

  Type fMinDistSQR = ezMath::BasicType<Type>::MaxValue();

  for (ezUInt32 i = 0; i < uiNumPoints; ++i)
  {
    const Type fDistSQR = (*pCur - m_vCenter).GetLengthSquared();

    fMinDistSQR = ezMath::Min(fMinDistSQR, fDistSQR);

    pCur = ezMemoryUtils::AddByteOffsetConst(pCur, uiStride);
  }

  return ezMath::Sqrt(fMinDistSQR);
}

template<typename Type>
bool ezBoundingSphereTemplate<Type>::GetRayIntersection(const ezVec3Template<Type>& vRayStartPos, const ezVec3Template<Type>& vRayDirNormalized, Type* out_fIntersection /* = nullptr */, ezVec3Template<Type>* out_vIntersection /* = nullptr */) const
{
  EZ_ASSERT_DEBUG(vRayDirNormalized.IsNormalized(), "The ray direction must be normalized.");

  // Ugly Code taken from 'Real Time Rendering First Edition' Page 299

  const Type fRadiusSQR = ezMath::Square(m_fRadius);
  const ezVec3Template<Type> vRelPos = m_vCenter - vRayStartPos;

  const Type d = vRelPos.Dot(vRayDirNormalized);
  const Type fRelPosLenSQR = vRelPos.GetLengthSquared();

  if (d < 0.0f && fRelPosLenSQR > fRadiusSQR)
    return false;

  const Type m2 = fRelPosLenSQR - ezMath::Square(d);

  if (m2 > fRadiusSQR)
    return false;

  const Type q = ezMath::Sqrt(fRadiusSQR - m2);

  Type fIntersectionTime;

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

template<typename Type>
bool ezBoundingSphereTemplate<Type>::GetLineSegmentIntersection(const ezVec3Template<Type>& vLineStartPos, const ezVec3Template<Type>& vLineEndPos, Type* out_fHitFraction /* = nullptr */, ezVec3Template<Type>* out_vIntersection /* = nullptr */) const
{
  Type fIntersection = 0.0f;

  const ezVec3Template<Type> vDir = vLineEndPos - vLineStartPos;
  ezVec3Template<Type> vDirNorm = vDir;
  const Type fLen = vDirNorm.GetLengthAndNormalize ();

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

#include <Foundation/Math/Implementation/AllClasses_inl.h>


