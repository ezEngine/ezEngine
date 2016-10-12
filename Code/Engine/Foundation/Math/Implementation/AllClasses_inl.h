#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Plane.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>

template<typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxTemplate<Type>::Contains(const ezBoundingSphereTemplate<Type>& sphere) const
{
  return Contains(sphere.GetBoundingBox());
}

template<typename Type>
EZ_FORCE_INLINE bool ezBoundingBoxTemplate<Type>::Overlaps(const ezBoundingSphereTemplate<Type>& sphere) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return sphere.Contains(GetClampedPoint(sphere.m_vCenter));
}

template<typename Type>
inline Type ezBoundingBoxTemplate<Type>::GetDistanceTo(const ezBoundingSphereTemplate<Type>& sphere) const
{
  return (GetClampedPoint(sphere.m_vCenter) - sphere.m_vCenter).GetLength() - sphere.m_fRadius;
}

template<typename Type>
inline const ezBoundingSphereTemplate<Type> ezBoundingBoxTemplate<Type>::GetBoundingSphere() const
{
  return ezBoundingSphereTemplate<Type>(GetCenter(), (m_vMax - m_vMin).GetLength() * (Type) 0.5);
}

template<typename Type>
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

template<typename Type>
Type ezBoundingSphereTemplate<Type>::GetDistanceTo(const ezBoundingBoxTemplate<Type>& rhs) const
{
  const ezVec3Template<Type> vPointOnBox = rhs.GetClampedPoint(m_vCenter);

  return GetDistanceTo(vPointOnBox);
}

template<typename Type>
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

template<typename Type>
bool ezBoundingSphereTemplate<Type>::Overlaps(const ezBoundingBoxTemplate<Type>& rhs) const
{
  return Contains(rhs.GetClampedPoint(m_vCenter));
}

template<typename Type>
const ezBoundingBoxTemplate<Type> ezBoundingSphereTemplate<Type>::GetBoundingBox() const
{
  return ezBoundingBoxTemplate<Type>(m_vCenter - ezVec3Template<Type>(m_fRadius), m_vCenter + ezVec3Template<Type>(m_fRadius));
}


template<typename Type>
ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetObjectPosition(const ezBoundingSphereTemplate<Type>& Sphere) const
{
  const Type fDist = GetDistanceTo(Sphere.m_vCenter);

  if (fDist >= Sphere.m_fRadius)
    return ezPositionOnPlane::Front;

  if (-fDist >= Sphere.m_fRadius)
    return ezPositionOnPlane::Back;

  return ezPositionOnPlane::Spanning;
}

template<typename Type>
ezPositionOnPlane::Enum ezPlaneTemplate<Type>::GetObjectPosition(const ezBoundingBoxTemplate<Type>& Box) const
{
  ezVec3Template<Type> vPos = Box.m_vMin;
  ezVec3Template<Type> vNeg = Box.m_vMax;

  if (m_vNormal.x >= (Type) 0)
  {
    vPos.x = Box.m_vMax.x;
    vNeg.x = Box.m_vMin.x;
  }

  if (m_vNormal.y >= (Type) 0)
  {
    vPos.y = Box.m_vMax.y;
    vNeg.y = Box.m_vMin.y;
  }

  if (m_vNormal.z >= (Type) 0)
  {
    vPos.z = Box.m_vMax.z;
    vNeg.z = Box.m_vMin.z;
  }

  if (GetDistanceTo(vPos) <= (Type) 0)
    return ezPositionOnPlane::Back;

  if (GetDistanceTo(vNeg) >= (Type) 0)
    return ezPositionOnPlane::Front;

  return ezPositionOnPlane::Spanning;
}




template<typename Type>
void ezMat3Template<Type>::SetRotationMatrix (const ezVec3Template<Type>& vAxis, ezAngle angle)
{
  EZ_ASSERT_DEBUG(vAxis.IsNormalized(), "vAxis must be normalized.");

  const Type cos = ezMath::Cos(angle);
  const Type sin = ezMath::Sin(angle);
  const Type oneminuscos = (Type) 1 - cos;

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

template<typename Type>
ezResult ezMat3Template<Type>::Invert(Type fEpsilon)
{
  const Type fDet = Element(0, 0) * (Element(2, 2) * Element(1, 1) - Element(1, 2) * Element(2, 1)) -
                     Element(0, 1) * (Element(2, 2) * Element(1, 0) - Element(1, 2) * Element(2, 0)) +
                     Element(0, 2) * (Element(2, 1) * Element(1, 0) - Element(1, 1) * Element(2, 0));

  if (ezMath::IsZero (fDet, fEpsilon))
    return EZ_FAILURE;

  const Type fOneDivDet = (Type) 1 / fDet;

  ezMat3Template<Type> Inverse;

  Inverse.Element(0, 0) = (Element(2, 2) * Element(1, 1) - Element(1, 2) * Element(2, 1));
  Inverse.Element(0, 1) =-(Element(2, 2) * Element(0, 1) - Element(0, 2) * Element(2, 1));
  Inverse.Element(0, 2) = (Element(1, 2) * Element(0, 1) - Element(0, 2) * Element(1, 1));

  Inverse.Element(1, 0) =-(Element(2, 2) * Element(1, 0) - Element(1, 2) * Element(2, 0));
  Inverse.Element(1, 1) = (Element(2, 2) * Element(0, 0) - Element(0, 2) * Element(2, 0));
  Inverse.Element(1, 2) =-(Element(1, 2) * Element(0, 0) - Element(0, 2) * Element(1, 0));

  Inverse.Element(2, 0) = (Element(2, 1) * Element(1, 0) - Element(1, 1) * Element(2, 0));
  Inverse.Element(2, 1) =-(Element(2, 1) * Element(0, 0) - Element(0, 1) * Element(2, 0));
  Inverse.Element(2, 2) = (Element(1, 1) * Element(0, 0) - Element(0, 1) * Element(1, 0));

  *this = Inverse * fOneDivDet;
  return EZ_SUCCESS;
}

template<typename Type>
void ezMat3Template<Type>::SetLookInDirectionMatrix (ezVec3Template<Type> vLookDir, ezVec3Template<Type> vUpDir)
{
  EZ_ASSERT_DEBUG(!vLookDir.IsZero(),"The look direction must not be zero.");
  EZ_ASSERT_DEBUG(vUpDir.IsNormalized(), "The up-direction must be normalized.");

  vLookDir.NormalizeIfNotZero();

  if (ezMath::Abs(vLookDir.Dot(vUpDir)) > (Type) 0.9999) // less than 1 degree difference -> problem
    vUpDir = vLookDir.GetOrthogonalVector();

  // ensure vUpDir is orthogonal to the looking direction
  const ezVec3Template<Type> vRightDir = vUpDir.Cross(vLookDir).GetNormalized ();

  // orthogonalize vUpDir
  vUpDir = vLookDir.Cross(vRightDir); // vLookDir and vRightDir are normalized and orthogonal, vUpDir will be normalized too

  // the matrix needs to be transposed (i.e. inverted), so we set the ROWS instead of the columns
  SetRow(0, vRightDir);
  SetRow(1, vUpDir);
  SetRow(2, vLookDir);

  // This code (now) uses a left-handed convention
}




template<typename Type>
void ezMat4Template<Type>::SetRotationMatrix(const ezVec3Template<Type>& vAxis, ezAngle angle)
{
  EZ_ASSERT_DEBUG(vAxis.IsNormalized(), "vAxis must be normalized.");

  const Type cos = ezMath::Cos(angle);
  const Type sin = ezMath::Sin(angle);
  const Type oneminuscos = (Type) 1 - cos;

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

template<typename Type>
ezResult ezMat4Template<Type>::Invert(Type fEpsilon)
{
  ezMat4Template<Type> Inverse;

  const Type fDet = GetDeterminantOf4x4Matrix (*this);

  if (ezMath::IsZero(fDet, fEpsilon))
    return EZ_FAILURE;

  Type fOneDivDet = ezMath::Invert(fDet);

  for (ezInt32 i = 0; i < 4; ++i)
  {

    Inverse.Element(i, 0) = GetDeterminantOf3x3SubMatrix (*this, i, 0) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 1) = GetDeterminantOf3x3SubMatrix (*this, i, 1) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 2) = GetDeterminantOf3x3SubMatrix (*this, i, 2) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 3) = GetDeterminantOf3x3SubMatrix (*this, i, 3) * fOneDivDet;
  }

  *this = Inverse;
  return EZ_SUCCESS;
}

template<typename Type>
void ezMat4Template<Type>::SetPerspectiveProjectionMatrixFromFovX(ezAngle fieldOfViewX, Type fAspectRatioWidthDivHeight, Type fNearZ, Type fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  const Type xm = fNearZ * ezMath::Tan(fieldOfViewX * 0.5f);
  const Type ym = xm / fAspectRatioWidthDivHeight;

  SetPerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange);
}

template<typename Type>
void ezMat4Template<Type>::SetPerspectiveProjectionMatrixFromFovY(ezAngle fieldOfViewY, Type fAspectRatioWidthDivHeight, Type fNearZ, Type fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  const Type ym = fNearZ * ezMath::Tan(fieldOfViewY * 0.5);
  const Type xm = ym * fAspectRatioWidthDivHeight;

  SetPerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange);
}

template<typename Type>
void ezMat4Template<Type>::SetPerspectiveProjectionMatrix(Type fViewWidth, Type fViewHeight, Type fNearZ, Type fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  SetPerspectiveProjectionMatrix(-fViewWidth * (Type) 0.5, fViewWidth * (Type) 0.5, -fViewHeight * (Type) 0.5, fViewHeight * (Type) 0.5, fNearZ, fFarZ, DepthRange);
}

template<typename Type>
void ezMat4Template<Type>::SetPerspectiveProjectionMatrix(Type fLeft, Type fRight, Type fBottom, Type fTop, Type fNearZ, Type fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  // This is an perspective projection matrix for a left-handed coordinate system
  // For both OpenGL and D3D we assume a left-handed coordinate system, even though right-handed is the 'default convention' in OpenGL
  // In OpenGL clip space goes from -1 to +1 (near plane at -1, far plane at +1)
  // In Direct3D clip space goes from 0 to +1 (near plane at 0, far plane at +1)
  // The depth-buffer value, however, will always be between 0 (near plane) and 1 (far plane), in both APIs.

  const Type fTwoNearZ = fNearZ + fNearZ;
  const Type fOneDivNearMinusFar = (Type)1 / (fNearZ - fFarZ);

  if (DepthRange == ezProjectionDepthRange::MinusOneToOne)
  {
    // The OpenGL Way (but LH): http://wiki.delphigl.com/index.php/glFrustum

    Element(2, 2) =   -(fFarZ + fNearZ) * fOneDivNearMinusFar;
    Element(3, 2) = 2 * fFarZ * fNearZ  * fOneDivNearMinusFar;
  }
  else
  {
    // The Direct3D Way: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205353(v=vs.85).aspx
    Element(2, 2) =   -(fFarZ +    0  ) * fOneDivNearMinusFar;
    Element(3, 2) =     fFarZ * fNearZ  * fOneDivNearMinusFar;
  }

  Element(0, 0) = fTwoNearZ / (fRight - fLeft);
  Element(1, 0) = (Type) 0;
  Element(2, 0) = (fRight + fLeft) / (fLeft - fRight);
  Element(3, 0) = (Type) 0;

  Element(0, 1) = (Type) 0;
  Element(1, 1) = fTwoNearZ / (fTop - fBottom);
  Element(2, 1) = (fTop + fBottom) / (fBottom - fTop);
  Element(3, 1) = (Type) 0;

  Element(0, 2) = (Type) 0;
  Element(1, 2) = (Type) 0;

  Element(0, 3) = (Type) 0;
  Element(1, 3) = (Type) 0;
  Element(2, 3) = (Type) 1;
  Element(3, 3) = (Type) 0;
}

template<typename Type>
void ezMat4Template<Type>::SetOrthographicProjectionMatrix(Type fViewWidth, Type fViewHeight, Type fNearZ, Type fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  SetOrthographicProjectionMatrix(-fViewWidth * (Type) 0.5, fViewWidth * (Type) 0.5, -fViewHeight * (Type) 0.5, fViewHeight * (Type) 0.5, fNearZ, fFarZ, DepthRange);
}

template<typename Type>
void ezMat4Template<Type>::SetOrthographicProjectionMatrix(Type fLeft, Type fRight, Type fBottom, Type fTop, Type fNearZ, Type fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  // This is an orthographic projection matrix for a left-handed coordinate system
  // For both OpenGL and D3D we assume a left-handed coordinate system, even though right-handed is the 'default convention' in OpenGL
  // In OpenGL clip space goes from -1 to +1 (near plane at -1, far plane at +1)
  // In Direct3D clip space goes from 0 to +1 (near plane at 0, far plane at +1)
  // The depth-buffer value, however, will always be between 0 (near plane) and 1 (far plane), in both APIs.

  const Type fOneDivFarMinusNear = (Type) 1 / (fFarZ - fNearZ);

  if (DepthRange == ezProjectionDepthRange::MinusOneToOne)
  {
    // The OpenGL Way (but LH): http://wiki.delphigl.com/index.php/glOrtho
    Element(2, 2) =  2 * fOneDivFarMinusNear;
    Element(3, 2) = -(fFarZ + fNearZ) * fOneDivFarMinusNear;
  }
  else
  {
    // The D3D Way: https://msdn.microsoft.com/en-us/library/windows/desktop/bb205347(v=vs.85).aspx
    Element(2, 2) =      fOneDivFarMinusNear;
    Element(3, 2) = -(  0   + fNearZ) * fOneDivFarMinusNear;
  }

  Element(0, 0) = (Type) 2 / (fRight - fLeft);
  Element(1, 0) = (Type) 0;
  Element(2, 0) = (Type) 0;
  Element(3, 0) = -(fRight + fLeft) / (fRight - fLeft);

  Element(0, 1) = (Type) 0;
  Element(1, 1) = (Type) 2 / (fTop - fBottom);
  Element(2, 1) = (Type) 0;
  Element(3, 1) = -(fTop + fBottom) / (fTop - fBottom);

  Element(0, 2) = (Type) 0;
  Element(1, 2) = (Type) 0;


  Element(0, 3) = (Type) 0;
  Element(1, 3) = (Type) 0;
  Element(2, 3) = (Type) 0;
  Element(3, 3) = (Type) 1;
}

template<typename Type>
void ezMat4Template<Type>::SetLookAtMatrix(const ezVec3Template<Type>& vStartPos, const ezVec3Template<Type>& vTargetPos, const ezVec3Template<Type>& vUpDir)
{
  ezMat3Template<Type> Rotation;
  Rotation.SetLookInDirectionMatrix(vTargetPos - vStartPos, vUpDir);

  SetRotationalPart(Rotation);
  SetTranslationVector(-(Rotation * vStartPos));
  SetRow(3, ezVec4Template<Type>(0, 0, 0, 1));
}


template<typename Type>
typename ezVec3Template<Type> ezVec3Template<Type>::CreateRandomPointInSphere(ezRandom& rng)
{
  /// \test This is new

  double px, py, pz;
  double len = 0.0;

  do
  {
    px = rng.DoubleMinMax(-1, 1);
    py = rng.DoubleMinMax(-1, 1);
    pz = rng.DoubleMinMax(-1, 1);

    len = (px * px) + (py * py) + (pz * pz);
  }
  while (len > 1.0 || len <= 0.000001); // prevent the exact center

  return ezVec3Template<Type>((Type)px, (Type)py, (Type)pz);
}

template<typename Type>
typename ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDirection(ezRandom& rng)
{
  /// \test This is new

  ezVec3Template<Type> vec = CreateRandomPointInSphere(rng);
  vec.Normalize();
  return vec;
}

template<typename Type>
typename ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationX(ezRandom& rng, const ezAngle& maxDeviation)
{
  /// \test This is new

  EZ_ASSERT_DEBUG(maxDeviation.GetRadian() > 0.0, "Deviation must not be zero");

  // two coordinates are always on a unit circle, the distance of the third coordinate defines the opening angle
  const double dist = 1.0 / ezMath::Tan(maxDeviation);

  double lenSqr;
  double p1;
  double p2;

  // create a random sample inside a unit sphere
  do
  {
    p1 = rng.DoubleMinMax(-1.0, 1.0);
    p2 = rng.DoubleMinMax(-1.0, 1.0);

    lenSqr = (p1 * p1 + p2 * p2);
  }
  while (lenSqr > 1.0);

  ezVec3Template<Type> vec((Type)dist, (Type)p1, (Type)p2);
  vec.Normalize();

  return vec;
}

template<typename Type>
typename ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationY(ezRandom& rng, const ezAngle& maxDeviation)
{
  /// \test This is new
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  ezMath::Swap(vec.x, vec.y);
  return vec;
}

template<typename Type>
typename ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviationZ(ezRandom& rng, const ezAngle& maxDeviation)
{
  /// \test This is new
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  ezMath::Swap(vec.x, vec.z);
  return vec;
}

template<typename Type>
typename ezVec3Template<Type> ezVec3Template<Type>::CreateRandomDeviation(ezRandom& rng, const ezAngle& maxDeviation, const ezVec3& vNormal)
{
  /// \test This is new

  // If you need to do this very often:
  // *** Pre-compute this once: ***

  // how to get from the X axis to our desired basis
  ezQuatTemplate<Type> qRotXtoDir;
  qRotXtoDir.SetShortestRotation(ezVec3Template<Type>(1, 0, 0), vNormal);

  // *** Then call this with the precomputed value as often as needed: ***

  // create a random vector along X
  ezVec3Template<Type> vec = CreateRandomDeviationX(rng, maxDeviation);
  // rotate from X to our basis
  return qRotXtoDir * vec;
}
