#include <Foundation/PCH.h>
#include <Foundation/Math/Frustum.h>


ezFrustum::ezFrustum()
{
  m_uiUsedPlanes = 0;
}

void ezFrustum::SetFrustum(const ezVec3& vPosition, ezUInt8 uiNumPlanes, const ezPlane* pPlanes)
{
  EZ_ASSERT(uiNumPlanes <= 16, "The frustum cannot have more than 16 planes.");

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
  /// \todo This can be optimized to only transform each vertex once and then check against all planes.

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
  /// \test Not tested yet.

  m_vPosition = vPosition;

	float t;
	float frustum[6][4];

  const float* Mat = ModelViewProjection.m_fElementsCM;

	// Extract the numbers for the RIGHT plane
	frustum[0][0] = Mat[12] - Mat[ 0];
	frustum[0][1] = Mat[13] - Mat[ 1];
	frustum[0][2] = Mat[15] - Mat[ 2];
	frustum[0][3] = Mat[15] - Mat[ 3];

	// Normalize the result
	t = 1.0f / ezMath::Sqrt (frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2]);
	frustum[0][0] *= t;
	frustum[0][1] *= t;
	frustum[0][2] *= t;
	frustum[0][3] *= t;

	// Extract the numbers for the LEFT plane
	frustum[1][0] = Mat[12] + Mat[ 0];
	frustum[1][1] = Mat[13] + Mat[ 1];
	frustum[1][2] = Mat[15] + Mat[ 2];
	frustum[1][3] = Mat[15] + Mat[ 3];

	// Normalize the result
	t = 1.0f / ezMath::Sqrt (frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2]);
	frustum[1][0] *= t;
	frustum[1][1] *= t;
	frustum[1][2] *= t;
	frustum[1][3] *= t;

	/* Extract the BOTTOM plane */
	frustum[2][0] = Mat[12] + Mat[ 4];
	frustum[2][1] = Mat[13] + Mat[ 5];
	frustum[2][2] = Mat[15] + Mat[ 6];
	frustum[2][3] = Mat[15] + Mat[ 7];

	/* Normalize the result */
	t = 1.0f / ezMath::Sqrt (frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2]);
	frustum[2][0] *= t;
	frustum[2][1] *= t;
	frustum[2][2] *= t;
	frustum[2][3] *= t;

	/* Extract the TOP plane */
	frustum[3][0] = Mat[12] - Mat[ 4];
	frustum[3][1] = Mat[13] - Mat[ 5];
	frustum[3][2] = Mat[15] - Mat[ 6];
	frustum[3][3] = Mat[15] - Mat[ 7];

	/* Normalize the result */
	t = 1.0f / ezMath::Sqrt (frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2]);
	frustum[3][0] *= t;
	frustum[3][1] *= t;
	frustum[3][2] *= t;
	frustum[3][3] *= t;

	/* Extract the FAR plane */
	frustum[4][0] = Mat[12] - Mat[ 8];
	frustum[4][1] = Mat[13] - Mat[ 9];
	frustum[4][2] = Mat[15] - Mat[10];
	frustum[4][3] = Mat[15] - Mat[11];

	/* Normalize the result */
	t = 1.0f / ezMath::Sqrt (frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2]);
	frustum[4][0] *= t;
	frustum[4][1] *= t;
	frustum[4][2] *= t;
	frustum[4][3] *= t;

	/* Extract the NEAR plane */
	frustum[5][0] = Mat[12] + Mat[ 8];
	frustum[5][1] = Mat[13] + Mat[ 9];
	frustum[5][2] = Mat[15] + Mat[10];
	frustum[5][3] = Mat[15] + Mat[11];

	/* Normalize the result */
	t = 1.0f / ezMath::Sqrt (frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2]);
	frustum[5][0] *= t;
	frustum[5][1] *= t;
	frustum[5][2] *= t;
	frustum[5][3] *= t;


  // Near Plane
	m_Planes[0].m_vNormal.x = frustum[5][0];
	m_Planes[0].m_vNormal.y = frustum[5][1];
	m_Planes[0].m_vNormal.z = frustum[5][2];

	// The near-plane has to be at the camera´s position, so that no portals in front of you, get culled away
  m_Planes[0].SetFromNormalAndPoint(m_Planes[0].m_vNormal, m_vPosition);

  // Right Plane
	m_Planes[1].m_vNormal.x    = frustum[0][0];
	m_Planes[1].m_vNormal.y    = frustum[0][1];
	m_Planes[1].m_vNormal.z    = frustum[0][2];
  m_Planes[1].m_fNegDistance = frustum[0][3];

  // Left Plane
	m_Planes[2].m_vNormal.x    = frustum[1][0];
	m_Planes[2].m_vNormal.y    = frustum[1][1];
	m_Planes[2].m_vNormal.z    = frustum[1][2];
	m_Planes[2].m_fNegDistance = frustum[1][3];

  // Bottom Plane
	m_Planes[3].m_vNormal.x    = frustum[2][0];
	m_Planes[3].m_vNormal.y    = frustum[2][1];
	m_Planes[3].m_vNormal.z    = frustum[2][2];
	m_Planes[3].m_fNegDistance = frustum[2][3];

  // Top Plane
	m_Planes[4].m_vNormal.x    = frustum[3][0];
	m_Planes[4].m_vNormal.y    = frustum[3][1];
	m_Planes[4].m_vNormal.z    = frustum[3][2];
	m_Planes[4].m_fNegDistance = frustum[3][3];

  // Far Plane
	m_Planes[5].m_vNormal.x    = frustum[4][0];
	m_Planes[5].m_vNormal.y    = frustum[4][1];
	m_Planes[5].m_vNormal.z    = frustum[4][2];
	m_Planes[5].m_fNegDistance = frustum[4][3];

  // move the far plane closer, if necessary
  {
    const float fTemp = m_Planes[5].GetDistanceTo(m_vPosition);

	  if (fTemp > fMaxFarPlaneDist)
		  m_Planes[5].m_fNegDistance += fMaxFarPlaneDist - fTemp;
  }
		
	m_uiUsedPlanes = 6;
}

void ezFrustum::SetFrustum (const ezVec3& vPosition, const ezVec3& vForwards, const ezVec3& vUp, ezAngle FovX, ezAngle FovY, float fFarPlane)
{
  /// \test Not tested yet.

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
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(-fCosFovY, 0, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[1].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }

  // Right Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3( fCosFovY, 0, fSinFovY);
    vPlaneNormal.Normalize();

    m_Planes[2].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }

  // Bottom Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0, -fCosFovX, fSinFovX);
    vPlaneNormal.Normalize();

    m_Planes[3].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }

  // Top Plane
  {
    ezVec3 vPlaneNormal = mLocalFrame * ezVec3(0,  fCosFovX, fSinFovX);
    vPlaneNormal.Normalize();

    m_Planes[4].SetFromNormalAndPoint(vPlaneNormal, m_vPosition);
  }
}




