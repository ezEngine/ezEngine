#pragma once

#include <Foundation/Math/Mat4.h>

EZ_FORCE_INLINE ezBoundingSphere::ezBoundingSphere()
{
#if EZ_COMPILE_FOR_DEBUG
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  // m_vCenter is already initialized to NaN by its own constructor.
  m_fRadius = ezMath::NaN();
#endif
}

EZ_FORCE_INLINE ezBoundingSphere::ezBoundingSphere(const ezVec3& vCenter, float fRadius)
{
  m_vCenter = vCenter;
  m_fRadius = fRadius;
}

inline void ezBoundingSphere::SetZero()
{
  m_vCenter.SetZero();
  m_fRadius = 0.0f;
}

inline bool ezBoundingSphere::IsZero(float fEpsilon /* = ezMath_DefaultEpsilon */) const
{
  return m_vCenter.IsZero(fEpsilon) && ezMath::IsZero(m_fRadius, fEpsilon);
}

inline void ezBoundingSphere::SetInvalid()
{
  m_vCenter.SetZero();
  m_fRadius = -1.0f;
}

inline bool ezBoundingSphere::IsValid() const
{
  return (m_vCenter.IsValid() && m_fRadius >= 0.0f);
}

EZ_FORCE_INLINE void ezBoundingSphere::SetElements(const ezVec3& vCenter, float fRadius)
{
  m_vCenter = vCenter;
  m_fRadius = fRadius;

  EZ_ASSERT(IsValid(), "The sphere was created with invalid values.");
}

inline void ezBoundingSphere::ExpandToInclude(const ezVec3& vPoint)
{
  const float fDistSQR = (vPoint - m_vCenter).GetLengthSquared();

  if (ezMath::Square(m_fRadius) < fDistSQR)
    m_fRadius = ezMath::Sqrt(fDistSQR);
}

inline void ezBoundingSphere::ExpandToInclude(const ezBoundingSphere& rhs)
{
  const float fReqRadius = (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius;

  m_fRadius = ezMath::Max(m_fRadius, fReqRadius);
}

EZ_FORCE_INLINE void ezBoundingSphere::Grow(float fDiff)
{
  EZ_ASSERT(IsValid(), "Cannot grow a sphere that is invalid.");

  m_fRadius += fDiff;

  EZ_ASSERT(IsValid(), "The grown sphere has become invalid.");
}

inline bool ezBoundingSphere::IsIdentical(const ezBoundingSphere& rhs) const
{
  return (m_vCenter.IsIdentical(rhs.m_vCenter) && m_fRadius == rhs.m_fRadius);
}

inline bool ezBoundingSphere::IsEqual(const ezBoundingSphere& rhs, float fEpsilon) const
{
  return (m_vCenter.IsEqual(rhs.m_vCenter, fEpsilon) && ezMath::IsFloatEqual(m_fRadius, rhs.m_fRadius, fEpsilon));
}

EZ_FORCE_INLINE bool operator== (const ezBoundingSphere& lhs, const ezBoundingSphere& rhs)
{
  return lhs.IsIdentical(rhs);
}

EZ_FORCE_INLINE bool operator!= (const ezBoundingSphere& lhs, const ezBoundingSphere& rhs)
{
  return !lhs.IsIdentical(rhs);
}

EZ_FORCE_INLINE void ezBoundingSphere::Translate(const ezVec3& vTranslation)
{
  m_vCenter += vTranslation;
}

EZ_FORCE_INLINE void ezBoundingSphere::ScaleFromCenter(float fScale)
{
  EZ_ASSERT(fScale >= 0.0f, "Cannot invert the sphere.");

  m_fRadius *= fScale;
}

inline void ezBoundingSphere::ScaleFromOrigin(const ezVec3& vScale)
{
  EZ_ASSERT(vScale.x >= 0.0f, "Cannot invert the sphere.");
  EZ_ASSERT(vScale.y >= 0.0f, "Cannot invert the sphere.");
  EZ_ASSERT(vScale.z >= 0.0f, "Cannot invert the sphere.");

  m_vCenter = m_vCenter.CompMult(vScale);

  // scale the radius by the maximum scaling factor (the sphere cannot become an elipsoid, 
  // so to be a 'bounding' sphere, it should be as large as possible
  m_fRadius *= ezMath::Max(vScale.x, vScale.y, vScale.z);
}

inline void ezBoundingSphere::TransformFromOrigin (const ezMat4& mTransform)
{
  m_vCenter = mTransform.TransformPosition (m_vCenter);

  const ezVec3 Scale = mTransform.GetScalingFactors ();
  m_fRadius *= ezMath::Max (Scale.x, Scale.y, Scale.z);
}

inline void ezBoundingSphere::TransformFromCenter (const ezMat4& mTransform)
{
  m_vCenter += mTransform.GetTranslationVector();

  const ezVec3 Scale = mTransform.GetScalingFactors ();
  m_fRadius *= ezMath::Max (Scale.x, Scale.y, Scale.z);
}

inline float ezBoundingSphere::GetDistanceTo(const ezVec3& vPoint) const
{
  return (vPoint - m_vCenter).GetLength() - m_fRadius;
}

inline float ezBoundingSphere::GetDistanceTo(const ezBoundingSphere& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() - m_fRadius - rhs.m_fRadius;
}

inline bool ezBoundingSphere::Contains(const ezVec3& vPoint) const
{
  return (vPoint - m_vCenter).GetLengthSquared() <= ezMath::Square(m_fRadius);
}

inline bool ezBoundingSphere::Contains(const ezBoundingSphere& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLength() + rhs.m_fRadius <= m_fRadius;
}

inline bool ezBoundingSphere::Overlaps(const ezBoundingSphere& rhs) const
{
  return (rhs.m_vCenter - m_vCenter).GetLengthSquared() < ezMath::Square(rhs.m_fRadius + m_fRadius);
}

inline const ezVec3 ezBoundingSphere::GetClampedPoint(const ezVec3& vPoint)
{
  const ezVec3 vDir = vPoint - m_vCenter;
  const float fDistSQR = vDir.GetLengthSquared ();
  
  // return the point, if it is already inside the sphere
  if (fDistSQR <= ezMath::Square (m_fRadius))
    return vPoint;

  // otherwise return a point on the surface of the sphere

  const float fLength = ezMath::Sqrt (fDistSQR);

  return m_vCenter + m_fRadius * (vDir / fLength);
}


