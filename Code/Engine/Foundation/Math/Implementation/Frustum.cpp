#include <Foundation/FoundationPCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/SimdMath/SimdBBox.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4f.h>
#include <Foundation/Utilities/GraphicsUtils.h>

ezFrustum::ezFrustum() = default;
ezFrustum::~ezFrustum() = default;

const ezPlane& ezFrustum::GetPlane(ezUInt8 uiPlane) const
{
  EZ_ASSERT_DEBUG(uiPlane < PLANE_COUNT, "Invalid plane index.");

  return m_Planes[uiPlane];
}

ezPlane& ezFrustum::AccessPlane(ezUInt8 uiPlane)
{
  EZ_ASSERT_DEBUG(uiPlane < PLANE_COUNT, "Invalid plane index.");

  return m_Planes[uiPlane];
}

bool ezFrustum::IsValid() const
{
  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    if (!m_Planes[i].IsValid())
      return false;
  }

  return true;
}

ezFrustum ezFrustum::MakeFromPlanes(const ezPlane* pPlanes)
{
  ezFrustum f;

  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
    f.m_Planes[i] = pPlanes[i];

  return f;
}

void ezFrustum::TransformFrustum(const ezMat4& mTransform)
{
  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
    m_Planes[i].Transform(mTransform);
}

ezVolumePosition::Enum ezFrustum::GetObjectPosition(const ezVec3* pVertices, ezUInt32 uiNumVertices) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const ezPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(pVertices, uiNumVertices);

    if (pos == ezPositionOnPlane::Back)
      continue;

    if (pos == ezPositionOnPlane::Front)
      return ezVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return ezVolumePosition::Intersecting;

  return ezVolumePosition::Inside;
}

static ezPositionOnPlane::Enum GetPlaneObjectPosition(const ezPlane& p, const ezVec3* const pPoints, ezUInt32 uiVertices, const ezMat4& mTransform)
{
  bool bFront = false;
  bool bBack = false;

  for (ezUInt32 i = 0; i < uiVertices; ++i)
  {
    switch (p.GetPointPosition(mTransform * pPoints[i]))
    {
      case ezPositionOnPlane::Front:
      {
        if (bBack)
          return ezPositionOnPlane::Spanning;

        bFront = true;
      }
      break;

      case ezPositionOnPlane::Back:
      {
        if (bFront)
          return (ezPositionOnPlane::Spanning);

        bBack = true;
      }
      break;

      default:
        break;
    }
  }

  return (bFront ? ezPositionOnPlane::Front : ezPositionOnPlane::Back);
}


ezVolumePosition::Enum ezFrustum::GetObjectPosition(const ezVec3* pVertices, ezUInt32 uiNumVertices, const ezMat4& mObjectTransform) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const ezPositionOnPlane::Enum pos = GetPlaneObjectPosition(m_Planes[i], pVertices, uiNumVertices, mObjectTransform);

    if (pos == ezPositionOnPlane::Back)
      continue;

    if (pos == ezPositionOnPlane::Front)
      return ezVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return ezVolumePosition::Intersecting;

  return ezVolumePosition::Inside;
}

ezVolumePosition::Enum ezFrustum::GetObjectPosition(const ezBoundingSphere& sphere) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const ezPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(sphere);

    if (pos == ezPositionOnPlane::Back)
      continue;

    if (pos == ezPositionOnPlane::Front)
      return ezVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return ezVolumePosition::Intersecting;

  return ezVolumePosition::Inside;
}

ezVolumePosition::Enum ezFrustum::GetObjectPosition(const ezBoundingBox& box) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const ezPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(box);

    if (pos == ezPositionOnPlane::Back)
      continue;

    if (pos == ezPositionOnPlane::Front)
      return ezVolumePosition::Outside;

    bOnSomePlane = true;
  }

  if (bOnSomePlane)
    return ezVolumePosition::Intersecting;

  return ezVolumePosition::Inside;
}

void ezFrustum::InvertFrustum()
{
  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
    m_Planes[i].Flip();
}

void ezFrustum::ComputeCornerPoints(ezVec3 out_pPoints[FrustumCorner::CORNER_COUNT]) const
{
  // clang-format off
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearTopLeft]).IgnoreResult();
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearTopRight]).IgnoreResult();
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::NearBottomLeft]).IgnoreResult();
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::NearBottomRight]).IgnoreResult();

  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarTopLeft]).IgnoreResult();
  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarTopRight]).IgnoreResult();
  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_pPoints[FrustumCorner::FarBottomLeft]).IgnoreResult();
  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_pPoints[FrustumCorner::FarBottomRight]).IgnoreResult();
  // clang-format on
}

ezFrustum ezFrustum::MakeFromMVP(const ezMat4& mModelViewProjection0, ezClipSpaceDepthRange::Enum depthRange, ezHandedness::Enum handedness)
{
  ezMat4 ModelViewProjection = mModelViewProjection0;
  ezGraphicsUtils::ConvertProjectionMatrixDepthRange(ModelViewProjection, depthRange, ezClipSpaceDepthRange::MinusOneToOne);

  ezVec4 planes[6];

  if (handedness == ezHandedness::LeftHanded)
  {
    ModelViewProjection.SetRow(0, -ModelViewProjection.GetRow(0));
  }

  planes[LeftPlane] = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(0);
  planes[RightPlane] = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(0);
  planes[BottomPlane] = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(1);
  planes[TopPlane] = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(1);
  planes[NearPlane] = -ModelViewProjection.GetRow(3) - ModelViewProjection.GetRow(2);
  planes[FarPlane] = -ModelViewProjection.GetRow(3) + ModelViewProjection.GetRow(2);

  // Normalize planes
  for (int p = 0; p < 6; ++p)
  {
    const float len = planes[p].GetAsVec3().GetLength();
    // doing the division here manually since we want to accept the case where length is 0 (infinite plane)
    const float invLen = 1.f / len;
    planes[p].x *= ezMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].y *= ezMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].z *= ezMath::IsFinite(invLen) ? invLen : 0.f;
    planes[p].w *= invLen;
  }

  // The last matrix row is giving the camera's plane, which means its normal is
  // also the camera's viewing direction.
  const ezVec3 cameraViewDirection = ModelViewProjection.GetRow(3).GetAsVec3();

  // Making sure the near/far plane is always closest/farthest. The way we derive the
  // planes always yields the closer plane pointing towards the camera and the farther
  // plane pointing away from the camera, so flip when that relationship inverts.
  if (planes[FarPlane].GetAsVec3().Dot(cameraViewDirection) < 0)
  {
    EZ_ASSERT_DEBUG(planes[NearPlane].GetAsVec3().Dot(cameraViewDirection) >= 0, "");
    ezMath::Swap(planes[NearPlane], planes[FarPlane]);
  }

  // In case we have an infinity far plane projection, the normal is invalid.
  // We'll just take the mirrored normal from the near plane.
  EZ_ASSERT_DEBUG(planes[NearPlane].IsValid(), "Near plane is expected to be non-nan and finite at this point!");
  if (ezMath::Abs(planes[FarPlane].w) == ezMath::Infinity<float>())
  {
    planes[FarPlane] = (-planes[NearPlane].GetAsVec3()).GetAsVec4(planes[FarPlane].w);
  }

  static_assert(offsetof(ezPlane, m_vNormal) == offsetof(ezVec4, x) && offsetof(ezPlane, m_fNegDistance) == offsetof(ezVec4, w));

  ezFrustum res;
  ezMemoryUtils::Copy(res.m_Planes, (ezPlane*)planes, 6);
  return res;
}

ezFrustum ezFrustum::MakeFromFOV(const ezVec3& vPosition, const ezVec3& vForwards, const ezVec3& vUp, ezAngle fovX, ezAngle fovY, float fNearPlane, float fFarPlane)
{
  EZ_ASSERT_DEBUG(ezMath::Abs(vForwards.GetNormalized().Dot(vUp.GetNormalized())) < 0.999f, "Up dir must be different from forward direction");

  const ezVec3 vForwardsNorm = vForwards.GetNormalized();
  const ezVec3 vRightNorm = vForwards.CrossRH(vUp).GetNormalized();
  const ezVec3 vUpNorm = vRightNorm.CrossRH(vForwards).GetNormalized();

  ezFrustum res;

  // Near Plane
  res.m_Planes[NearPlane].SetFromNormalAndPoint(-vForwardsNorm, vPosition + fNearPlane * vForwardsNorm);

  // Far Plane
  res.m_Planes[FarPlane].SetFromNormalAndPoint(vForwardsNorm, vPosition + fFarPlane * vForwardsNorm);

  // Making sure the near/far plane is always closest/farthest.
  if (fNearPlane > fFarPlane)
  {
    ezMath::Swap(res.m_Planes[NearPlane], res.m_Planes[FarPlane]);
  }

  ezMat3 mLocalFrame;
  mLocalFrame.SetColumn(0, vRightNorm);
  mLocalFrame.SetColumn(1, vUpNorm);
  mLocalFrame.SetColumn(2, -vForwardsNorm);

  const float fCosFovX = ezMath::Cos(fovX * 0.5f);
  const float fSinFovX = ezMath::Sin(fovX * 0.5f);

  const float fCosFovY = ezMath::Cos(fovY * 0.5f);
  const float fSinFovY = ezMath::Sin(fovY * 0.5f);

  // Left Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(-fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    res.m_Planes[LeftPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Right Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    res.m_Planes[RightPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Bottom Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0, -fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    res.m_Planes[BottomPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Top Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0, fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    res.m_Planes[TopPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  return res;
}

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Frustum);
