#include <FoundationPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

ezResult ezGraphicsUtils::ConvertWorldPosToScreenPos(const ezMat4& ModelViewProjection, const ezUInt32 uiViewportX,
  const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vPoint, ezVec3& out_vScreenPos,
  ezClipSpaceDepthRange::Enum DepthRange)
{
  const ezVec4 vToProject = vPoint.GetAsVec4(1.0f);

  ezVec4 vClipSpace = ModelViewProjection * vToProject;

  if (vClipSpace.w == 0.0f)
    return EZ_FAILURE;

  ezVec3 vProjected = vClipSpace.GetAsVec3() / vClipSpace.w;
  if (vClipSpace.w < 0.0f)
    vProjected.z = -vProjected.z;

  out_vScreenPos.x = uiViewportX + uiViewportWidth * ((vProjected.x * 0.5f) + 0.5f);
  out_vScreenPos.y = uiViewportY + uiViewportHeight * ((vProjected.y * 0.5f) + 0.5f);

  // normalize the output z value to always be in [0; 1] range
  // That means when the projection matrix spits out values between -1 and +1, rescale those values
  if (DepthRange == ezClipSpaceDepthRange::MinusOneToOne)
    out_vScreenPos.z = vProjected.z * 0.5f + 0.5f;
  else
    out_vScreenPos.z = vProjected.z;

  return EZ_SUCCESS;
}

ezResult ezGraphicsUtils::ConvertScreenPosToWorldPos(const ezMat4& InverseModelViewProjection, const ezUInt32 uiViewportX,
  const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint,
  ezVec3* out_vDirection, ezClipSpaceDepthRange::Enum DepthRange)
{
  ezVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (DepthRange == ezClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  ezVec4 vToUnProject = vClipSpace.GetAsVec4(1.0f);

  ezVec4 vWorldSpacePoint = InverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0f)
    return EZ_FAILURE;

  out_vPoint = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;

  if (out_vDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const ezVec4 vWorldSpacePoint2 = InverseModelViewProjection * vToUnProject;

    EZ_ASSERT_DEV(vWorldSpacePoint2.w != 0.0f,
      "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const ezVec3 vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    *out_vDirection = (vPoint2 - out_vPoint).GetNormalized();
  }

  return EZ_SUCCESS;
}

ezResult ezGraphicsUtils::ConvertScreenPosToWorldPos(const ezMat4d& InverseModelViewProjection, const ezUInt32 uiViewportX,
  const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint,
  ezVec3* out_vDirection /*= nullptr*/, ezClipSpaceDepthRange::Enum DepthRange /*= ezClipSpaceDepthRange::Default*/)
{
  ezVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (DepthRange == ezClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  ezVec4d vToUnProject = ezVec4d(vClipSpace.x, vClipSpace.y, vClipSpace.z, 1.0);

  ezVec4d vWorldSpacePoint = InverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0)
    return EZ_FAILURE;

  ezVec3d outTemp = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;
  out_vPoint.Set((float)outTemp.x, (float)outTemp.y, (float)outTemp.z);

  if (out_vDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const ezVec4d vWorldSpacePoint2 = InverseModelViewProjection * vToUnProject;

    EZ_ASSERT_DEV(vWorldSpacePoint2.w != 0.0,
      "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const ezVec3d vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    ezVec3d outDir = (vPoint2 - outTemp).GetNormalized();
    out_vDirection->Set((float)outDir.x, (float)outDir.y, (float)outDir.z);
  }

  return EZ_SUCCESS;
}

bool ezGraphicsUtils::IsTriangleFlipRequired(const ezMat3& mTransformation)
{
  return (mTransformation.GetColumn(0).CrossRH(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);
}

void ezGraphicsUtils::ConvertProjectionMatrixDepthRange(
  ezMat4& inout_Matrix, ezClipSpaceDepthRange::Enum SrcDepthRange, ezClipSpaceDepthRange::Enum DstDepthRange)
{
  // exclude identity transformations
  if (SrcDepthRange == DstDepthRange)
    return;

  ezVec4 row2 = inout_Matrix.GetRow(2);
  ezVec4 row3 = inout_Matrix.GetRow(3);

  // only need to check SrcDepthRange, the rest is the logical conclusion from being not equal
  if (SrcDepthRange == ezClipSpaceDepthRange::MinusOneToOne /*&& DstDepthRange == ezClipSpaceDepthRange::ZeroToOne*/)
  {
    // map z => (z + w)/2
    row2 += row3;
    row2 *= 0.5f;
  }
  else // if (SrcDepthRange == ezClipSpaceDepthRange::ZeroToOne && DstDepthRange == ezClipSpaceDepthRange::MinusOneToOne)
  {
    // map z => 2z - w
    row2 += row2;
    row2 -= row3;
  }


  inout_Matrix.SetRow(2, row2);
  inout_Matrix.SetRow(3, row3);
}

void ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const ezMat4& ProjectionMatrix, ezAngle& out_fFovX, ezAngle& out_fFovY)
{
  const ezVec3 row0 = ProjectionMatrix.GetRow(0).GetAsVec3();
  const ezVec3 row1 = ProjectionMatrix.GetRow(1).GetAsVec3();
  const ezVec3 row2 = ProjectionMatrix.GetRow(2).GetAsVec3();
  const ezVec3 row3 = ProjectionMatrix.GetRow(3).GetAsVec3();

  const ezVec3 leftPlane = (row3 + row0).GetNormalized();
  const ezVec3 rightPlane = (row3 - row0).GetNormalized();
  const ezVec3 bottomPlane = (row3 + row1).GetNormalized();
  const ezVec3 topPlane = (row3 - row1).GetNormalized();

  out_fFovX = ezAngle::Radian(ezMath::Pi<float>()) - ezMath::ACos(leftPlane.Dot(rightPlane));
  out_fFovY = ezAngle::Radian(ezMath::Pi<float>()) - ezMath::ACos(topPlane.Dot(bottomPlane));
}

void ezGraphicsUtils::ExtractNearAndFarClipPlaneDistances(
  float& out_fNear, float& out_fFar, const ezMat4& ProjectionMatrix, ezClipSpaceDepthRange::Enum DepthRange)
{
  const ezVec4 row2 = ProjectionMatrix.GetRow(2);
  const ezVec4 row3 = ProjectionMatrix.GetRow(3);

  ezVec4 nearPlane = row2;

  if (DepthRange == ezClipSpaceDepthRange::MinusOneToOne)
  {
    nearPlane += row3;
  }

  out_fNear = ezMath::Abs(nearPlane.w / nearPlane.GetAsVec3().GetLength());

  const ezVec4 farPlane = row3 - row2;
  out_fFar = farPlane.w / farPlane.GetAsVec3().GetLength();
}

ezPlane ezGraphicsUtils::ComputeInterpolatedFrustumPlane(
  FrustumPlaneInterpolation direction, float fLerpFactor, const ezMat4& ProjectionMatrix, ezClipSpaceDepthRange::Enum DepthRange)
{
  ezVec4 rowA;
  ezVec4 rowB = ProjectionMatrix.GetRow(3);
  const float factorMinus1to1 = (fLerpFactor - 0.5f) * 2.0f; // bring into [-1; +1] range

  switch (direction)
  {
    case FrustumPlaneInterpolation::LeftToRight:
    {
      rowA = ProjectionMatrix.GetRow(0);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::BottomToTop:
    {
      rowA = ProjectionMatrix.GetRow(1);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::NearToFar:
      rowA = ProjectionMatrix.GetRow(2);

      if (DepthRange == ezClipSpaceDepthRange::ZeroToOne)
        rowB *= fLerpFactor; // [0; 1] range
      else
        rowB *= factorMinus1to1;
      break;
  }

  ezPlane res;
  res.m_vNormal = rowA.GetAsVec3() - rowB.GetAsVec3();
  res.m_fNegDistance = (rowA.w - rowB.w) / res.m_vNormal.GetLengthAndNormalize();

  return res;
}

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
  ezClipSpaceDepthRange::Enum DepthRange, ezClipSpaceYMode::Enum yRange, ezHandedness::Enum handedness)
{
  const float vw = fViewWidth * 0.5f;
  const float vh = fViewHeight * 0.5f;

  return CreatePerspectiveProjectionMatrix(-vw, vw, -vh, vh, fNearZ, fFarZ, DepthRange, yRange, handedness);
}

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(ezAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ,
  float fFarZ, ezClipSpaceDepthRange::Enum DepthRange, ezClipSpaceYMode::Enum yRange, ezHandedness::Enum handedness)
{
  const float xm = fNearZ * ezMath::Tan(fieldOfViewX * 0.5f);
  const float ym = xm / fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange, yRange, handedness);
}

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(ezAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ,
  float fFarZ, ezClipSpaceDepthRange::Enum DepthRange, ezClipSpaceYMode::Enum yRange, ezHandedness::Enum handedness)
{
  const float ym = fNearZ * ezMath::Tan(fieldOfViewY * 0.5);
  const float xm = ym * fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, DepthRange, yRange, handedness);
}

ezMat4 ezGraphicsUtils::CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ,
  ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum yRange, ezHandedness::Enum handedness)
{
  return CreateOrthographicProjectionMatrix(
    -fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, depthRange, yRange, handedness);
}

ezMat4 ezGraphicsUtils::CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
  ezClipSpaceDepthRange::Enum DepthRange, ezClipSpaceYMode::Enum yRange, ezHandedness::Enum handedness)
{
  ezMat4 res;
  res.SetIdentity();

  if (yRange == ezClipSpaceYMode::Flipped)
  {
    ezMath::Swap(fBottom, fTop);
  }

  const float fOneDivFarMinusNear = 1.0f / (fFarZ - fNearZ);
  const float fOneDivRightMinusLeft = 1.0f / (fRight - fLeft);
  const float fOneDivTopMinusBottom = 1.0f / (fTop - fBottom);

  res.Element(0, 0) = 2.0f / (fRight - fLeft);

  res.Element(1, 1) = 2.0f / (fTop - fBottom);

  res.Element(3, 0) = -(fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(3, 1) = -(fTop + fBottom) * fOneDivTopMinusBottom;


  if (DepthRange == ezClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    res.Element(2, 2) = -2.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -(fFarZ + fNearZ) * fOneDivFarMinusNear;
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh

    res.Element(2, 2) = -1.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -fNearZ * fOneDivFarMinusNear;
  }

  if (handedness == ezHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ,
  ezClipSpaceDepthRange::Enum DepthRange, ezClipSpaceYMode::Enum yRange, ezHandedness::Enum handedness)
{
  ezMat4 res;
  res.SetZero();

  if (yRange == ezClipSpaceYMode::Flipped)
  {
    ezMath::Swap(fBottom, fTop);
  }

  const float fTwoNearZ = fNearZ + fNearZ;
  const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);
  const float fOneDivRightMinusLeft = 1.0f / (fRight - fLeft);
  const float fOneDivTopMinusBottom = 1.0f / (fTop - fBottom);

  res.Element(0, 0) = fTwoNearZ * fOneDivRightMinusLeft;

  res.Element(1, 1) = fTwoNearZ * fOneDivTopMinusBottom;

  res.Element(2, 0) = (fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(2, 1) = (fTop + fBottom) * fOneDivTopMinusBottom;
  res.Element(2, 3) = -1.0f;

  if (DepthRange == ezClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    res.Element(2, 2) = (fFarZ + fNearZ) * fOneDivNearMinusFar;
    res.Element(3, 2) = 2 * fFarZ * fNearZ * fOneDivNearMinusFar;
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/en-us/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterrh
    res.Element(2, 2) = fFarZ * fOneDivNearMinusFar;
    res.Element(3, 2) = fFarZ * fNearZ * fOneDivNearMinusFar;
  }

  if (handedness == ezHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

ezMat3 ezGraphicsUtils::CreateLookAtViewMatrix(const ezVec3& vTarget, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  EZ_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  ezVec3 vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(ezVec3::UnitXAxis());

  ezVec3 vNormalizedUpDir = vUpDir.GetNormalized();

  if (ezMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.9999f) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  ezMat3 res;

  const ezVec3 zaxis = (handedness == ezHandedness::RightHanded) ? -vLookDir : vLookDir;
  const ezVec3 xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const ezVec3 yaxis = zaxis.CrossRH(xaxis);

  res.SetRow(0, xaxis);
  res.SetRow(1, yaxis);
  res.SetRow(2, zaxis);

  return res;
}

ezMat3 ezGraphicsUtils::CreateInverseLookAtViewMatrix(const ezVec3& vTarget, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  EZ_ASSERT_DEBUG(!vTarget.IsZero(), "The target must not be at the origin.");

  ezVec3 vLookDir = vTarget;
  vLookDir.NormalizeIfNotZero(ezVec3::UnitXAxis());

  ezVec3 vNormalizedUpDir = vUpDir.GetNormalized();

  if (ezMath::Abs(vLookDir.Dot(vNormalizedUpDir)) > 0.9999f) // less than 1 degree difference -> problem
  {
    // use some arbitrary other orthogonal vector as UP
    vNormalizedUpDir = vLookDir.GetOrthogonalVector();
  }

  ezMat3 res;

  const ezVec3 zaxis = (handedness == ezHandedness::RightHanded) ? -vLookDir : vLookDir;
  const ezVec3 xaxis = vNormalizedUpDir.CrossRH(zaxis).GetNormalized();
  const ezVec3 yaxis = zaxis.CrossRH(xaxis);

  res.SetColumn(0, xaxis);
  res.SetColumn(1, yaxis);
  res.SetColumn(2, zaxis);

  return res;
}

ezMat4 ezGraphicsUtils::CreateLookAtViewMatrix(
  const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  const ezMat3 rotation = CreateLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  ezMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(rotation * -vEyePos);
  res.SetRow(3, ezVec4(0, 0, 0, 1));
  return res;
}

ezMat4 ezGraphicsUtils::CreateInverseLookAtViewMatrix(
  const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  const ezMat3 rotation = CreateInverseLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  ezMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(vEyePos);
  res.SetRow(3, ezVec4(0, 0, 0, 1));
  return res;
}

ezMat4 ezGraphicsUtils::CreateViewMatrix(
  const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  ezMat4 res;
  res.SetIdentity();

  ezVec3 xaxis, yaxis, zaxis;

  if (handedness == ezHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetRow(0, xaxis.GetAsVec4(0));
  res.SetRow(1, yaxis.GetAsVec4(0));
  res.SetRow(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(ezVec3(-xaxis.Dot(vPosition), -yaxis.Dot(vPosition), -zaxis.Dot(vPosition)));

  return res;
}

ezMat4 ezGraphicsUtils::CreateInverseViewMatrix(
  const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  ezMat4 res;
  res.SetIdentity();

  ezVec3 xaxis, yaxis, zaxis;

  if (handedness == ezHandedness::LeftHanded)
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = vForwardDir;
  }
  else
  {
    xaxis = vRightDir;
    yaxis = vUpDir;
    zaxis = -vForwardDir;
  }

  res.SetColumn(0, xaxis.GetAsVec4(0));
  res.SetColumn(1, yaxis.GetAsVec4(0));
  res.SetColumn(2, zaxis.GetAsVec4(0));
  res.SetTranslationVector(vPosition);

  return res;
}

void ezGraphicsUtils::DecomposeViewMatrix(
  ezVec3& vPosition, ezVec3& vForwardDir, ezVec3& vRightDir, ezVec3& vUpDir, const ezMat4& viewMatrix, ezHandedness::Enum handedness)
{
  const ezMat3 rotation = viewMatrix.GetRotationalPart();

  if (handedness == ezHandedness::LeftHanded)
  {
    vRightDir = rotation.GetRow(0);
    vUpDir = rotation.GetRow(1);
    vForwardDir = rotation.GetRow(2);
  }
  else
  {
    vRightDir = rotation.GetRow(0);
    vUpDir = rotation.GetRow(1);
    vForwardDir = -rotation.GetRow(2);
  }

  vPosition = rotation.GetTranspose() * -viewMatrix.GetTranslationVector();
}

EZ_STATICLINK_FILE(Foundation, Foundation_Utilities_Implementation_GraphicsUtils);
