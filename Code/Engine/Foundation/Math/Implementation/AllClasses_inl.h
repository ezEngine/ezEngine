#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>

template <typename Type>
EZ_ALWAYS_INLINE bool ezBoundingBoxTemplate<Type>::Contains(const ezBoundingSphereTemplate<Type>& sphere) const
{
  return Contains(sphere.GetBoundingBox());
}

template <typename Type>
EZ_ALWAYS_INLINE bool ezBoundingBoxTemplate<Type>::Overlaps(const ezBoundingSphereTemplate<Type>& sphere) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return sphere.Contains(GetClampedPoint(sphere.m_vCenter));
}

template <typename Type>
inline Type ezBoundingBoxTemplate<Type>::GetDistanceTo(const ezBoundingSphereTemplate<Type>& sphere) const
{
  return (GetClampedPoint(sphere.m_vCenter) - sphere.m_vCenter).GetLength() - sphere.m_fRadius;
}

template <typename Type>
inline const ezBoundingSphereTemplate<Type> ezBoundingBoxTemplate<Type>::GetBoundingSphere() const
{
  return ezBoundingSphereTemplate<Type>(GetCenter(), (m_vMax - m_vMin).GetLength() * (Type)0.5);
}

template <typename Type>
void ezBoundingSphereTemplate<Type>::ExpandToInclude(const ezBoundingBoxTemplate<Type>& rhs)
{
  // compute the min and max extends of the AABB relative to the sphere (sphere center is the new origin)
  const ezVec3 vDiffMax = rhs.m_vMax - m_vCenter;
  const ezVec3 vDiffMin = rhs.m_vMin - m_vCenter;

  // compute the absolute distance to each AABB extremum, per axis
  const ezVec3 vDiffMaxAbs(ezMath::Abs(vDiffMax.x), ezMath::Abs(vDiffMax.y), ezMath::Abs(vDiffMax.z));
  const ezVec3 vDiffMinAbs(ezMath::Abs(vDiffMin.x), ezMath::Abs(vDiffMin.y), ezMath::Abs(vDiffMin.z));

  // take the maximum distance for each axis, to compute the point that is the farthest away from the sphere
  const ezVec3 vMostDistantPoint = vDiffMinAbs.CompMax(vDiffMaxAbs);

  const Type fDistSQR = vMostDistantPoint.GetLengthSquared();

  if (ezMath::Square(m_fRadius) < fDistSQR)
    m_fRadius = ezMath::Sqrt(fDistSQR);
}

template <typename Type>
Type ezBoundingSphereTemplate<Type>::GetDistanceTo(const ezBoundingBoxTemplate<Type>& rhs) const
{
  const ezVec3Template<Type> vPointOnBox = rhs.GetClampedPoint(m_vCenter);

  return GetDistanceTo(vPointOnBox);
}

template <typename Type>
bool ezBoundingSphereTemplate<Type>::Contains(const ezBoundingBoxTemplate<Type>& rhs) const
{
  // compute the min and max extends of the AABB relative to the sphere (sphere center is the new origin)
  const ezVec3 vDiffMax = rhs.m_vMax - m_vCenter;
  const ezVec3 vDiffMin = rhs.m_vMin - m_vCenter;

  // compute the absolute distance to each AABB extremum, per axis
  const ezVec3 vDiffMaxAbs(ezMath::Abs(vDiffMax.x), ezMath::Abs(vDiffMax.y), ezMath::Abs(vDiffMax.z));
  const ezVec3 vDiffMinAbs(ezMath::Abs(vDiffMin.x), ezMath::Abs(vDiffMin.y), ezMath::Abs(vDiffMin.z));

  // take the maximum distance for each axis, to compute the point that is the farthest away from the sphere
  const ezVec3 vMostDistantPoint = vDiffMinAbs.CompMax(vDiffMaxAbs);

  // if the squared length of that point is still smaller than the sphere radius, it is inside the sphere
  // and thus the whole AABB is inside the sphere
  return vMostDistantPoint.GetLengthSquared() <= m_fRadius * m_fRadius;
}

template <typename Type>
bool ezBoundingSphereTemplate<Type>::Overlaps(const ezBoundingBoxTemplate<Type>& rhs) const
{
  return Contains(rhs.GetClampedPoint(m_vCenter));
}

template <typename Type>
const ezBoundingBoxTemplate<Type> ezBoundingSphereTemplate<Type>::GetBoundingBox() const
{
  return ezBoundingBoxTemplate<Type>(m_vCenter - ezVec3Template<Type>(m_fRadius), m_vCenter + ezVec3Template<Type>(m_fRadius));
}


template <typename Type>
ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetObjectPosition(const ezBoundingSphereTemplate<Type>& Sphere) const
{
  const Type fDist = GetDistanceTo(Sphere.m_vCenter);

  if (fDist >= Sphere.m_fRadius)
    return ezPositionOnPlane::Front;

  if (-fDist >= Sphere.m_fRadius)
    return ezPositionOnPlane::Back;

  return ezPositionOnPlane::Spanning;
}

template <typename Type>
ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetObjectPosition(const ezBoundingBoxTemplate<Type>& Box) const
{
  ezVec3Template<Type> vPos = Box.m_vMin;
  ezVec3Template<Type> vNeg = Box.m_vMax;

  if (m_vNormal.x >= (Type)0)
  {
    vPos.x = Box.m_vMax.x;
    vNeg.x = Box.m_vMin.x;
  }

  if (m_vNormal.y >= (Type)0)
  {
    vPos.y = Box.m_vMax.y;
    vNeg.y = Box.m_vMin.y;
  }

  if (m_vNormal.z >= (Type)0)
  {
    vPos.z = Box.m_vMax.z;
    vNeg.z = Box.m_vMin.z;
  }

  if (GetDistanceTo(vPos) <= (Type)0)
    return ezPositionOnPlane::Back;

  if (GetDistanceTo(vNeg) >= (Type)0)
    return ezPositionOnPlane::Front;

  return ezPositionOnPlane::Spanning;
}



template <typename Type>
void ezMat3Template<Type>::SetRotationMatrix(const ezVec3Template<Type>& vAxis, ezAngle angle)
{
  EZ_ASSERT_DEBUG(vAxis.IsNormalized(0.1f), "vAxis must be normalized.");

  const Type cos = ezMath::Cos(angle);
  const Type sin = ezMath::Sin(angle);
  const Type oneminuscos = (Type)1 - cos;

  const Type xy = vAxis.x * vAxis.y;
  const Type xz = vAxis.x * vAxis.z;
  const Type yz = vAxis.y * vAxis.z;

  const Type xsin = vAxis.x * sin;
  const Type ysin = vAxis.y * sin;
  const Type zsin = vAxis.z * sin;

  const Type onecos_xy = oneminuscos * xy;
  const Type onecos_xz = oneminuscos * xz;
  const Type onecos_yz = oneminuscos * yz;

  //Column 1
  Element(0, 0) = cos + (oneminuscos * (vAxis.x * vAxis.x));
  Element(0, 1) = onecos_xy + zsin;
  Element(0, 2) = onecos_xz - ysin;

  //Column 2  )
  Element(1, 0) = onecos_xy - zsin;
  Element(1, 1) = cos + (oneminuscos * (vAxis.y * vAxis.y));
  Element(1, 2) = onecos_yz + xsin;

  //Column 3  )
  Element(2, 0) = onecos_xz + ysin;
  Element(2, 1) = onecos_yz - xsin;
  Element(2, 2) = cos + (oneminuscos * (vAxis.z * vAxis.z));
}

template <typename Type>
ezResult ezMat3Template<Type>::Invert(Type fEpsilon)
{
  const Type fDet = Element(0, 0) * (Element(2, 2) * Element(1, 1) - Element(1, 2) * Element(2, 1)) -
                    Element(0, 1) * (Element(2, 2) * Element(1, 0) - Element(1, 2) * Element(2, 0)) +
                    Element(0, 2) * (Element(2, 1) * Element(1, 0) - Element(1, 1) * Element(2, 0));

  if (ezMath::IsZero(fDet, fEpsilon))
    return EZ_FAILURE;

  const Type fOneDivDet = (Type)1 / fDet;

  ezMat3Template<Type> Inverse;

  Inverse.Element(0, 0) = (Element(2, 2) * Element(1, 1) - Element(1, 2) * Element(2, 1));
  Inverse.Element(0, 1) = -(Element(2, 2) * Element(0, 1) - Element(0, 2) * Element(2, 1));
  Inverse.Element(0, 2) = (Element(1, 2) * Element(0, 1) - Element(0, 2) * Element(1, 1));

  Inverse.Element(1, 0) = -(Element(2, 2) * Element(1, 0) - Element(1, 2) * Element(2, 0));
  Inverse.Element(1, 1) = (Element(2, 2) * Element(0, 0) - Element(0, 2) * Element(2, 0));
  Inverse.Element(1, 2) = -(Element(1, 2) * Element(0, 0) - Element(0, 2) * Element(1, 0));

  Inverse.Element(2, 0) = (Element(2, 1) * Element(1, 0) - Element(1, 1) * Element(2, 0));
  Inverse.Element(2, 1) = -(Element(2, 1) * Element(0, 0) - Element(0, 1) * Element(2, 0));
  Inverse.Element(2, 2) = (Element(1, 1) * Element(0, 0) - Element(0, 1) * Element(1, 0));

  *this = Inverse * fOneDivDet;
  return EZ_SUCCESS;
}

template <typename Type>
void ezMat4Template<Type>::SetRotationMatrix(const ezVec3Template<Type>& vAxis, ezAngle angle)
{
  EZ_ASSERT_DEBUG(vAxis.IsNormalized(), "vAxis must be normalized.");

  const Type cos = ezMath::Cos(angle);
  const Type sin = ezMath::Sin(angle);
  const Type oneminuscos = (Type)1 - cos;

  const Type xy = vAxis.x * vAxis.y;
  const Type xz = vAxis.x * vAxis.z;
  const Type yz = vAxis.y * vAxis.z;

  const Type xsin = vAxis.x * sin;
  const Type ysin = vAxis.y * sin;
  const Type zsin = vAxis.z * sin;

  const Type onecos_xy = oneminuscos * xy;
  const Type onecos_xz = oneminuscos * xz;
  const Type onecos_yz = oneminuscos * yz;

  //Column 1
  Element(0, 0) = cos + (oneminuscos * (vAxis.x * vAxis.x));
  Element(0, 1) = onecos_xy + zsin;
  Element(0, 2) = onecos_xz - ysin;
  Element(0, 3) = 0;

  //Column 2
  Element(1, 0) = onecos_xy - zsin;
  Element(1, 1) = cos + (oneminuscos * (vAxis.y * vAxis.y));
  Element(1, 2) = onecos_yz + xsin;
  Element(1, 3) = 0;

  //Column 3
  Element(2, 0) = onecos_xz + ysin;
  Element(2, 1) = onecos_yz - xsin;
  Element(2, 2) = cos + (oneminuscos * (vAxis.z * vAxis.z));
  Element(2, 3) = 0;

  //Column 4
  Element(3, 0) = 0;
  Element(3, 1) = 0;
  Element(3, 2) = 0;
  Element(3, 3) = 1;
}

template <typename Type>
ezResult ezMat4Template<Type>::Invert(Type fEpsilon)
{
  ezMat4Template<Type> Inverse;

  const Type fDet = GetDeterminantOf4x4Matrix(*this);

  if (ezMath::IsZero(fDet, fEpsilon))
    return EZ_FAILURE;

  Type fOneDivDet = ezMath::Invert(fDet);

  for (ezInt32 i = 0; i < 4; ++i)
  {

    Inverse.Element(i, 0) = GetDeterminantOf3x3SubMatrix(*this, i, 0) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 1) = GetDeterminantOf3x3SubMatrix(*this, i, 1) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 2) = GetDeterminantOf3x3SubMatrix(*this, i, 2) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 3) = GetDeterminantOf3x3SubMatrix(*this, i, 3) * fOneDivDet;
  }

  *this = Inverse;
  return EZ_SUCCESS;
}
