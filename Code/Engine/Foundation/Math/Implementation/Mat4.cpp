#include <Foundation/PCH.h>
#include <Foundation/Math/Mat4.h>

void ezMat4::SetRotationMatrix (const ezVec3& vAxis, float fAngle)
{
  EZ_ASSERT(vAxis.IsNormalized(), "vAxis must be normalized.");

  const float rad = ezMath::DegToRad(fAngle);
  const float cos = ezMath::CosRad (rad);
  const float sin = ezMath::SinRad (rad);
  const float oneminuscos = 1 - cos;

  const float xy = vAxis.x * vAxis.y;
  const float xz = vAxis.x * vAxis.z;
  const float yz = vAxis.y * vAxis.z;

  const float xsin = vAxis.x * sin;
  const float ysin = vAxis.y * sin;
  const float zsin = vAxis.z * sin;

  const float onecos_xy = oneminuscos * xy;
  const float onecos_xz = oneminuscos * xz;
  const float onecos_yz = oneminuscos * yz;

  //Column 1
  m_fColumn[0][0] = cos + (oneminuscos * (vAxis.x * vAxis.x));
  m_fColumn[0][1] = onecos_xy + zsin;
  m_fColumn[0][2] = onecos_xz - ysin;
  m_fColumn[0][3] = 0;
             
  //Column 2
  m_fColumn[1][0] = onecos_xy - zsin;
  m_fColumn[1][1] = cos + (oneminuscos * (vAxis.y * vAxis.y));
  m_fColumn[1][2] = onecos_yz + xsin;
  m_fColumn[1][3] = 0;
             
  //Column 3
  m_fColumn[2][0] = onecos_xz + ysin;
  m_fColumn[2][1] = onecos_yz - xsin;
  m_fColumn[2][2] = cos + (oneminuscos * (vAxis.z * vAxis.z));
  m_fColumn[2][3] = 0;
             
  //Column 4
  m_fColumn[3][0] = 0;
  m_fColumn[3][1] = 0;
  m_fColumn[3][2] = 0;
  m_fColumn[3][3] = 1;
}

ezResult ezMat4::Invert(float fEpsilon)
{
  ezMat4 Inverse;

  const float fDet = GetDeterminantOf4x4Matrix (*this);

  if (ezMath::IsZero(fDet, fEpsilon))
    return EZ_FAILURE;

  float fOneDivDet = ezMath::Invert(fDet);

  for (ezInt32 i = 0; i < 4; ++i)
  {
      
    Inverse.m_fColumn[i][0] = GetDeterminantOf3x3SubMatrix (*this, i, 0) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.m_fColumn[i][1] = GetDeterminantOf3x3SubMatrix (*this, i, 1) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.m_fColumn[i][2] = GetDeterminantOf3x3SubMatrix (*this, i, 2) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.m_fColumn[i][3] = GetDeterminantOf3x3SubMatrix (*this, i, 3) * fOneDivDet;
  }

  *this = Inverse;
  return EZ_SUCCESS;
}


void ezMat4::SetPerspectiveProjectionMatrixFromFovX (float fFieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  const float xm = fNearZ * ezMath::TanDeg (fFieldOfViewX * 0.5f);
  const float ym = xm / fAspectRatioWidthDivHeight;

  SetPerspectiveProjectionMatrix (-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange);
}

void ezMat4::SetPerspectiveProjectionMatrixFromFovY (float fFieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  const float ym = fNearZ * ezMath::TanDeg (fFieldOfViewY * 0.5f);
  const float xm = ym * fAspectRatioWidthDivHeight;

  SetPerspectiveProjectionMatrix (-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange);
}

void ezMat4::SetPerspectiveProjectionMatrix (float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  SetPerspectiveProjectionMatrix (-fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, DepthRange);
}

void ezMat4::SetPerspectiveProjectionMatrix (float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  const float fTwoNearZ = fNearZ + fNearZ;
  const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);

  if (DepthRange == ezProjectionDepthRange::MinusOneToOne)
  {
    m_fColumn[2][2] = -(fFarZ + fNearZ) * fOneDivNearMinusFar;
    m_fColumn[3][2] = 2 * fFarZ * fNearZ * fOneDivNearMinusFar;
  }
  else
  {
    m_fColumn[2][2] = -(fFarZ +    0  ) * fOneDivNearMinusFar;
    m_fColumn[3][2] =    fFarZ * fNearZ * fOneDivNearMinusFar;
  }

  m_fColumn[0][0] = fTwoNearZ / (fRight - fLeft);
  m_fColumn[1][0] = 0.0f;
  m_fColumn[2][0] = (fLeft + fRight) / (fLeft - fRight);
  m_fColumn[3][0] = 0.0f;
             
  m_fColumn[0][1] = 0.0f;
  m_fColumn[1][1] = fTwoNearZ / (fTop - fBottom);
  m_fColumn[2][1] = (fBottom + fTop) / (fBottom - fTop);
  m_fColumn[3][1] = 0.0f;
             
  m_fColumn[0][2] = 0.0f;
  m_fColumn[1][2] = 0.0f;
             
  m_fColumn[0][3] = 0.0f;
  m_fColumn[1][3] = 0.0f;
  m_fColumn[2][3] = 1.0f;
  m_fColumn[3][3] = 0.0f;
}


void ezMat4::SetOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  SetOrthographicProjectionMatrix(-fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, DepthRange);
}


void ezMat4::SetOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, ezProjectionDepthRange::Enum DepthRange)
{
  const float fOneDivFarMinusNear = 1.0f / (fFarZ - fNearZ);

  if (DepthRange == ezProjectionDepthRange::MinusOneToOne)
  {
    m_fColumn[2][2] =  2 * fOneDivFarMinusNear;
    m_fColumn[3][2] = -(fFarZ + fNearZ) * fOneDivFarMinusNear;
  }
  else
  {
    m_fColumn[2][2] =      fOneDivFarMinusNear;
    m_fColumn[3][2] = -(  0   + fNearZ) * fOneDivFarMinusNear;
  }

  m_fColumn[0][0] = 2.0f / (fRight - fLeft);
  m_fColumn[1][0] = 0.0f;
  m_fColumn[2][0] = 0.0f;
  m_fColumn[3][0] = (fRight + fLeft) / (fRight - fLeft);
             
  m_fColumn[0][1] = 0.0f;
  m_fColumn[1][1] = 2.0f / (fTop - fBottom);
  m_fColumn[2][1] = 0.0f;
  m_fColumn[3][1] = (fTop + fBottom) / (fTop - fBottom);
             
  m_fColumn[0][2] = 0.0f;
  m_fColumn[1][2] = 0.0f;

             
  m_fColumn[0][3] = 0.0f;
  m_fColumn[1][3] = 0.0f;
  m_fColumn[2][3] = 0.0f;
  m_fColumn[3][3] = 1.0f;
}

void ezMat4::SetLookAtMatrix(const ezVec3& vStartPos, const ezVec3& vTargetPos, const ezVec3& vUpDir)
{
  ezMat3 Rotation;
  Rotation.SetLookInDirectionMatrix (vTargetPos - vStartPos, vUpDir);

  SetRotationalPart(Rotation);
  SetTranslationVector(vStartPos);
  SetRow(3, ezVec4(0, 0, 0, 1));
}