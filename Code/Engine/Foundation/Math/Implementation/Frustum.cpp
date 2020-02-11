#include <FoundationPCH.h>

#include <Foundation/Math/Frustum.h>
#include <SimdMath/SimdBBox.h>
#include <SimdMath/SimdConversion.h>
#include <SimdMath/SimdVec4f.h>
#include <Utilities/GraphicsUtils.h>

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

void ezFrustum::SetFrustum(const ezPlane* pPlanes)
{
  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
    m_Planes[i] = pPlanes[i];
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

static ezPositionOnPlane::Enum GetPlaneObjectPosition(
  const ezPlane& p, const ezVec3* const vPoints, ezUInt32 iVertices, const ezMat4& mTransform)
{
  bool bFront = false;
  bool bBack = false;

  for (ezUInt32 i = 0; i < iVertices; ++i)
  {
    switch (p.GetPointPosition(mTransform * vPoints[i]))
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

ezVolumePosition::Enum ezFrustum::GetObjectPosition(const ezBoundingSphere& Sphere) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const ezPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(Sphere);

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

ezVolumePosition::Enum ezFrustum::GetObjectPosition(const ezBoundingBox& Box) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (ezUInt32 i = 0; i < PLANE_COUNT; ++i)
  {
    const ezPositionOnPlane::Enum pos = m_Planes[i].GetObjectPosition(Box);

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

void ezFrustum::ComputeCornerPoints(ezVec3 out_Points[FrustumCorner::CORNER_COUNT]) const
{
  // clang-format off
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_Points[FrustumCorner::NearTopLeft]);
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_Points[FrustumCorner::NearTopRight]);
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_Points[FrustumCorner::NearBottomLeft]);
  ezPlane::GetPlanesIntersectionPoint(m_Planes[NearPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_Points[FrustumCorner::NearBottomRight]);

  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[LeftPlane], out_Points[FrustumCorner::FarTopLeft]);
  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[TopPlane], m_Planes[RightPlane], out_Points[FrustumCorner::FarTopRight]);
  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[LeftPlane], out_Points[FrustumCorner::FarBottomLeft]);
  ezPlane::GetPlanesIntersectionPoint(m_Planes[FarPlane], m_Planes[BottomPlane], m_Planes[RightPlane], out_Points[FrustumCorner::FarBottomRight]);
  // clang-format on
}

void ezFrustum::SetFrustum(const ezMat4& ModelViewProjection0, ezClipSpaceDepthRange::Enum DepthRange, ezHandedness::Enum Handedness)
{
  ezMat4 ModelViewProjection = ModelViewProjection0;
  ezGraphicsUtils::ConvertProjectionMatrixDepthRange(ModelViewProjection, DepthRange, ezClipSpaceDepthRange::MinusOneToOne);

  ezVec4 planes[6];

  if (Handedness == ezHandedness::LeftHanded)
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
    planes[p] /= len;

    m_Planes[p].m_vNormal = planes[p].GetAsVec3();
    m_Planes[p].m_fNegDistance = planes[p].w;
  }
}

void ezFrustum::SetFrustum(
  const ezVec3& vPosition, const ezVec3& vForwards, const ezVec3& vUp, ezAngle FovX, ezAngle FovY, float fNearPlane, float fFarPlane)
{
  EZ_ASSERT_DEBUG(
    ezMath::Abs(vForwards.GetNormalized().Dot(vUp.GetNormalized())) < 0.999f, "Up dir must be different from forward direction");

  const ezVec3 vForwardsNorm = vForwards.GetNormalized();
  const ezVec3 vRightNorm = vForwards.CrossRH(vUp).GetNormalized();
  const ezVec3 vUpNorm = vRightNorm.CrossRH(vForwards).GetNormalized();

  // Near Plane
  m_Planes[NearPlane].SetFromNormalAndPoint(-vForwardsNorm, vPosition + fNearPlane * vForwardsNorm);

  // Far Plane
  m_Planes[FarPlane].SetFromNormalAndPoint(vForwardsNorm, vPosition + fFarPlane * vForwardsNorm);

  ezMat3 mLocalFrame;
  mLocalFrame.SetColumn(0, vRightNorm);
  mLocalFrame.SetColumn(1, vUpNorm);
  mLocalFrame.SetColumn(2, -vForwardsNorm);

  const float fCosFovX = ezMath::Cos(FovX * 0.5f);
  const float fSinFovX = ezMath::Sin(FovX * 0.5f);

  const float fCosFovY = ezMath::Cos(FovY * 0.5f);
  const float fSinFovY = ezMath::Sin(FovY * 0.5f);

  // Left Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(-fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    m_Planes[LeftPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Right Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    m_Planes[RightPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Bottom Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0, -fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[BottomPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }

  // Top Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0, fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[TopPlane].SetFromNormalAndPoint(vPlaneNormal, vPosition);
  }
}

EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Frustum);
