#include <Foundation/PCH.h>
#include <Foundation/Math/Frustum.h>


ezFrustum::ezFrustum()
{
  m_uiUsedPlanes = 0;
  m_vPosition.SetZero();
}

void ezFrustum::SetFrustum(const ezVec3& vPosition, ezUInt8 uiNumPlanes, const ezPlane* pPlanes)
{
  EZ_ASSERT_DEBUG(uiNumPlanes <= 16, "The frustum cannot have more than 16 planes.");

  m_vPosition = vPosition;

  m_uiUsedPlanes = uiNumPlanes;

  for (ezUInt32 i = 0; i < uiNumPlanes; ++i)
    m_Planes[i] = pPlanes[i];
}

void ezFrustum::TransformFrustum(const ezMat4& mTransform)
{
  m_vPosition = mTransform * m_vPosition;

  for (ezUInt32 i = 0; i < m_uiUsedPlanes; ++i)
    m_Planes[i].Transform(mTransform);
}

ezVolumePosition::Enum ezFrustum::GetObjectPosition(const ezVec3* pVertices, ezUInt32 uiNumVertices) const
{
  /// \test Not yet tested

  bool bOnSomePlane = false;

  for (ezUInt32 i = 0; i < m_uiUsedPlanes; ++i)
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

static ezPositionOnPlane::Enum GetPlaneObjectPosition(const ezPlane& p, const ezVec3* const vPoints, ezUInt32 iVertices, const ezMat4& mTransform)
{
  bool bFront = false;
  bool bBack = false;

  for (ezUInt32 i = 0; i < iVertices; ++i)
  {
    switch (p.GetPointPosition (mTransform * vPoints[i]))
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

  for (ezUInt32 i = 0; i < m_uiUsedPlanes; ++i)
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

  for (ezUInt32 i = 0; i < m_uiUsedPlanes; ++i)
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

  for (ezUInt32 i = 0; i < m_uiUsedPlanes; ++i)
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
  for (ezUInt32 i = 0; i < m_uiUsedPlanes; ++i)
    m_Planes[i].Flip();
}

void ezFrustum::SetFrustum(const ezVec3& vPosition, const ezMat4& ModelViewProjection, float fMaxFarPlaneDist)
{
  /// \test Tested manually, works as expected, but no automatic tests yet.

  m_vPosition = vPosition;

  // Near Plane
  {
    m_Planes[0].m_vNormal.x = ModelViewProjection.Element(0, 3) - ModelViewProjection.Element(0, 2);
    m_Planes[0].m_vNormal.y = ModelViewProjection.Element(1, 3) - ModelViewProjection.Element(1, 2);
    m_Planes[0].m_vNormal.z = ModelViewProjection.Element(2, 3) - ModelViewProjection.Element(2, 2);
    m_Planes[0].m_vNormal.Normalize();

    // The near-plane has to be at the camera's position, so that no portals in front of you, get culled away
    m_Planes[0].SetFromNormalAndPoint(m_Planes[0].m_vNormal, vPosition);
  }

  // Far Plane
  {
    m_Planes[5].m_vNormal.x     = -(ModelViewProjection.Element(0, 3) - ModelViewProjection.Element(0, 2));
    m_Planes[5].m_vNormal.y     = -(ModelViewProjection.Element(1, 3) - ModelViewProjection.Element(1, 2));
    m_Planes[5].m_vNormal.z     = -(ModelViewProjection.Element(2, 3) - ModelViewProjection.Element(2, 2));
    m_Planes[5].m_fNegDistance  = -(ModelViewProjection.Element(3, 3) - ModelViewProjection.Element(3, 2));
    m_Planes[5].m_fNegDistance /= m_Planes[5].m_vNormal.GetLengthAndNormalize();

    // move the far plane closer, if necessary
    {
      const float fTemp = -m_Planes[5].GetDistanceTo(m_vPosition);

      if (fTemp > fMaxFarPlaneDist)
        m_Planes[5].m_fNegDistance += fTemp - fMaxFarPlaneDist;
    }
  }

  // Right Plane
  {
    m_Planes[1].m_vNormal.x     = +ModelViewProjection.Element(0, 0) - ModelViewProjection.Element(0, 3);
    m_Planes[1].m_vNormal.y     = +ModelViewProjection.Element(1, 0) - ModelViewProjection.Element(1, 3);
    m_Planes[1].m_vNormal.z     = +ModelViewProjection.Element(2, 0) - ModelViewProjection.Element(2, 3);
    m_Planes[1].m_fNegDistance  = +ModelViewProjection.Element(3, 0) - ModelViewProjection.Element(3, 3);
    m_Planes[1].m_fNegDistance /= m_Planes[1].m_vNormal.GetLengthAndNormalize();
  }

  // Left Plane
  {
    m_Planes[2].m_vNormal.x     = -ModelViewProjection.Element(0, 0) - ModelViewProjection.Element(0, 3);
    m_Planes[2].m_vNormal.y     = -ModelViewProjection.Element(1, 0) - ModelViewProjection.Element(1, 3);
    m_Planes[2].m_vNormal.z     = -ModelViewProjection.Element(2, 0) - ModelViewProjection.Element(2, 3);
    m_Planes[2].m_fNegDistance  = -ModelViewProjection.Element(3, 0) - ModelViewProjection.Element(3, 3);
    m_Planes[2].m_fNegDistance /= m_Planes[2].m_vNormal.GetLengthAndNormalize();
  }

  // Top Plane
  {
    m_Planes[3].m_vNormal.x     = +ModelViewProjection.Element(0, 1) - ModelViewProjection.Element(0, 3);
    m_Planes[3].m_vNormal.y     = +ModelViewProjection.Element(1, 1) - ModelViewProjection.Element(1, 3);
    m_Planes[3].m_vNormal.z     = +ModelViewProjection.Element(2, 1) - ModelViewProjection.Element(2, 3);
    m_Planes[3].m_fNegDistance  = +ModelViewProjection.Element(3, 1) - ModelViewProjection.Element(3, 3);
    m_Planes[3].m_fNegDistance /= m_Planes[3].m_vNormal.GetLengthAndNormalize();
  }

  // Bottom Plane
  {
    m_Planes[4].m_vNormal.x     = -ModelViewProjection.Element(0, 1) - ModelViewProjection.Element(0, 3);
    m_Planes[4].m_vNormal.y     = -ModelViewProjection.Element(1, 1) - ModelViewProjection.Element(1, 3);
    m_Planes[4].m_vNormal.z     = -ModelViewProjection.Element(2, 1) - ModelViewProjection.Element(2, 3);
    m_Planes[4].m_fNegDistance  = -ModelViewProjection.Element(3, 1) - ModelViewProjection.Element(3, 3);
    m_Planes[4].m_fNegDistance /= m_Planes[4].m_vNormal.GetLengthAndNormalize();
  }

  m_uiUsedPlanes = 6;
}

void ezFrustum::SetFrustum(const ezVec3& vPosition, const ezVec3& vForwards, const ezVec3& vUp, ezAngle FovX, ezAngle FovY, float fFarPlane)
{
  /// \test Tested manually, works as expected, but no automatic tests yet.

  m_vPosition = vPosition;
  
  const ezVec3 vForwardsNorm = vForwards.GetNormalized();
  const ezVec3 vRightNorm = vForwards.Cross(vUp).GetNormalized();
  const ezVec3 vUpNorm = vRightNorm.Cross(vForwards).GetNormalized();

  m_uiUsedPlanes = 6;

  // Near Plane
  m_Planes[0].SetFromNormalAndPoint(-vForwardsNorm, m_vPosition);

  // Far Plane
  m_Planes[5].SetFromNormalAndPoint(vForwardsNorm, m_vPosition + fFarPlane * vForwardsNorm);

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

    m_Planes[1].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }

  // Right Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3( fCosFovX, 0, fSinFovX);
    vPlaneNormal.Normalize();

    m_Planes[2].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }

  // Bottom Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0, -fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[3].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }

  // Top Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0,  fCosFovY, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[4].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }
}






EZ_STATICLINK_FILE(Foundation, Foundation_Math_Implementation_Frustum);

