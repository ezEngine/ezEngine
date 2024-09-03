#include <Foundation/FoundationPCH.h>

#include <Foundation/Utilities/GraphicsUtils.h>

ezResult ezGraphicsUtils::ConvertWorldPosToScreenPos(const ezMat4& mModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vPoint, ezVec3& out_vScreenPos, ezClipSpaceDepthRange::Enum depthRange)
{
  const ezVec4 vToProject = vPoint.GetAsVec4(1.0f);

  ezVec4 vClipSpace = mModelViewProjection * vToProject;

  if (vClipSpace.w == 0.0f)
    return EZ_FAILURE;

  ezVec3 vProjected = vClipSpace.GetAsVec3() / vClipSpace.w;
  if (vClipSpace.w < 0.0f)
    vProjected.z = -vProjected.z;

  out_vScreenPos.x = uiViewportX + uiViewportWidth * ((vProjected.x * 0.5f) + 0.5f);
  out_vScreenPos.y = uiViewportY + uiViewportHeight * ((vProjected.y * 0.5f) + 0.5f);

  // normalize the output z value to always be in [0; 1] range
  // That means when the projection matrix spits out values between -1 and +1, rescale those values
  if (depthRange == ezClipSpaceDepthRange::MinusOneToOne)
    out_vScreenPos.z = vProjected.z * 0.5f + 0.5f;
  else
    out_vScreenPos.z = vProjected.z;

  return EZ_SUCCESS;
}

ezResult ezGraphicsUtils::ConvertScreenPosToWorldPos(
  const ezMat4& mInverseModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint, ezVec3* out_pDirection, ezClipSpaceDepthRange::Enum depthRange)
{
  ezVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (depthRange == ezClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  ezVec4 vToUnProject = vClipSpace.GetAsVec4(1.0f);

  ezVec4 vWorldSpacePoint = mInverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0f)
    return EZ_FAILURE;

  out_vPoint = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;

  if (out_pDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const ezVec4 vWorldSpacePoint2 = mInverseModelViewProjection * vToUnProject;

    EZ_ASSERT_DEV(vWorldSpacePoint2.w != 0.0f, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const ezVec3 vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    *out_pDirection = (vPoint2 - out_vPoint).GetNormalized();
  }

  return EZ_SUCCESS;
}

ezResult ezGraphicsUtils::ConvertScreenPosToWorldPos(const ezMat4d& mInverseModelViewProjection, const ezUInt32 uiViewportX, const ezUInt32 uiViewportY, const ezUInt32 uiViewportWidth, const ezUInt32 uiViewportHeight, const ezVec3& vScreenPos, ezVec3& out_vPoint, ezVec3* out_pDirection /*= nullptr*/,
  ezClipSpaceDepthRange::Enum depthRange /*= ezClipSpaceDepthRange::Default*/)
{
  ezVec3 vClipSpace = vScreenPos;

  // From window coordinates to [0; 1] range
  vClipSpace.x = (vClipSpace.x - uiViewportX) / uiViewportWidth;
  vClipSpace.y = (vClipSpace.y - uiViewportY) / uiViewportHeight;

  // Map to range [-1; 1]
  vClipSpace.x = vClipSpace.x * 2.0f - 1.0f;
  vClipSpace.y = vClipSpace.y * 2.0f - 1.0f;

  // The OpenGL matrix expects the z values to be between -1 and +1, so rescale the incoming value to that range
  if (depthRange == ezClipSpaceDepthRange::MinusOneToOne)
    vClipSpace.z = vClipSpace.z * 2.0f - 1.0f;

  ezVec4d vToUnProject = ezVec4d(vClipSpace.x, vClipSpace.y, vClipSpace.z, 1.0);

  ezVec4d vWorldSpacePoint = mInverseModelViewProjection * vToUnProject;

  if (vWorldSpacePoint.w == 0.0)
    return EZ_FAILURE;

  ezVec3d outTemp = vWorldSpacePoint.GetAsVec3() / vWorldSpacePoint.w;
  out_vPoint.Set((float)outTemp.x, (float)outTemp.y, (float)outTemp.z);

  if (out_pDirection != nullptr)
  {
    vToUnProject.z += 0.1f; // a point that is a bit further away

    const ezVec4d vWorldSpacePoint2 = mInverseModelViewProjection * vToUnProject;

    EZ_ASSERT_DEV(vWorldSpacePoint2.w != 0.0, "It should not be possible that the first projected point has a w other than zero, but the second one has!");

    const ezVec3d vPoint2 = vWorldSpacePoint2.GetAsVec3() / vWorldSpacePoint2.w;

    ezVec3d outDir = (vPoint2 - outTemp).GetNormalized();
    out_pDirection->Set((float)outDir.x, (float)outDir.y, (float)outDir.z);
  }

  return EZ_SUCCESS;
}

bool ezGraphicsUtils::IsTriangleFlipRequired(const ezMat3& mTransformation)
{
  return (mTransformation.GetColumn(0).CrossRH(mTransformation.GetColumn(1)).Dot(mTransformation.GetColumn(2)) < 0.0f);
}

void ezGraphicsUtils::ConvertProjectionMatrixDepthRange(ezMat4& inout_mMatrix, ezClipSpaceDepthRange::Enum srcDepthRange, ezClipSpaceDepthRange::Enum dstDepthRange)
{
  // exclude identity transformations
  if (srcDepthRange == dstDepthRange)
    return;

  ezVec4 row2 = inout_mMatrix.GetRow(2);
  ezVec4 row3 = inout_mMatrix.GetRow(3);

  // only need to check SrcDepthRange, the rest is the logical conclusion from being not equal
  if (srcDepthRange == ezClipSpaceDepthRange::MinusOneToOne /*&& DstDepthRange == ezClipSpaceDepthRange::ZeroToOne*/)
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


  inout_mMatrix.SetRow(2, row2);
  inout_mMatrix.SetRow(3, row3);
}

void ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const ezMat4& mProjectionMatrix, ezAngle& out_fovX, ezAngle& out_fovY)
{

  const ezVec3 row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const ezVec3 row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const ezVec3 row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const ezVec3 leftPlane = (row3 + row0).GetNormalized();
  const ezVec3 rightPlane = (row3 - row0).GetNormalized();
  const ezVec3 bottomPlane = (row3 + row1).GetNormalized();
  const ezVec3 topPlane = (row3 - row1).GetNormalized();

  out_fovX = ezAngle::MakeFromRadian(ezMath::Pi<float>()) - ezMath::ACos(leftPlane.Dot(rightPlane));
  out_fovY = ezAngle::MakeFromRadian(ezMath::Pi<float>()) - ezMath::ACos(topPlane.Dot(bottomPlane));
}

void ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const ezMat4& mProjectionMatrix, ezAngle& out_fovLeft, ezAngle& out_fovRight, ezAngle& out_fovBottom, ezAngle& out_fovTop, ezClipSpaceYMode::Enum range)
{
  const ezVec3 row0 = mProjectionMatrix.GetRow(0).GetAsVec3();
  const ezVec3 row1 = mProjectionMatrix.GetRow(1).GetAsVec3();
  const ezVec3 row3 = mProjectionMatrix.GetRow(3).GetAsVec3();

  const ezVec3 leftPlane = (row3 + row0).GetNormalized();
  const ezVec3 rightPlane = (row3 - row0).GetNormalized();
  const ezVec3 bottomPlane = (row3 + row1).GetNormalized();
  const ezVec3 topPlane = (row3 - row1).GetNormalized();

  out_fovLeft = -ezMath::ACos(leftPlane.Dot(ezVec3(1.0f, 0, 0)));
  out_fovRight = ezAngle::MakeFromRadian(ezMath::Pi<float>()) - ezMath::ACos(rightPlane.Dot(ezVec3(1.0f, 0, 0)));
  out_fovBottom = -ezMath::ACos(bottomPlane.Dot(ezVec3(0, 1.0f, 0)));
  out_fovTop = ezAngle::MakeFromRadian(ezMath::Pi<float>()) - ezMath::ACos(topPlane.Dot(ezVec3(0, 1.0f, 0)));

  if (range == ezClipSpaceYMode::Flipped)
    ezMath::Swap(out_fovBottom, out_fovTop);
}

ezResult ezGraphicsUtils::ExtractPerspectiveMatrixFieldOfView(const ezMat4& mProjectionMatrix, float& out_fLeft, float& out_fRight, float& out_fBottom, float& out_fTop, ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum range)
{
  float fNear, fFar;
  EZ_SUCCEED_OR_RETURN(ExtractNearAndFarClipPlaneDistances(fNear, fFar, mProjectionMatrix, depthRange));
  // Compensate for inverse-Z.
  const float fMinDepth = ezMath::Min(fNear, fFar);

  ezAngle fFovLeft;
  ezAngle fFovRight;
  ezAngle fFovBottom;
  ezAngle fFovTop;
  ExtractPerspectiveMatrixFieldOfView(mProjectionMatrix, fFovLeft, fFovRight, fFovBottom, fFovTop, range);

  out_fLeft = ezMath::Tan(fFovLeft) * fMinDepth;
  out_fRight = ezMath::Tan(fFovRight) * fMinDepth;
  out_fBottom = ezMath::Tan(fFovBottom) * fMinDepth;
  out_fTop = ezMath::Tan(fFovTop) * fMinDepth;
  return EZ_SUCCESS;
}

ezResult ezGraphicsUtils::ExtractNearAndFarClipPlaneDistances(float& out_fNear, float& out_fFar, const ezMat4& mProjectionMatrix, ezClipSpaceDepthRange::Enum depthRange)
{
  const ezVec4 row2 = mProjectionMatrix.GetRow(2);
  const ezVec4 row3 = mProjectionMatrix.GetRow(3);

  ezVec4 nearPlane = row2;

  if (depthRange == ezClipSpaceDepthRange::MinusOneToOne)
  {
    nearPlane += row3;
  }

  const ezVec4 farPlane = row3 - row2;

  const float nearLength = nearPlane.GetAsVec3().GetLength();
  const float farLength = farPlane.GetAsVec3().GetLength();

  const float nearW = ezMath::Abs(nearPlane.w);
  const float farW = ezMath::Abs(farPlane.w);

  if ((nearLength < ezMath::SmallEpsilon<float>() && farLength < ezMath::SmallEpsilon<float>()) ||
      nearW < ezMath::SmallEpsilon<float>() || farW < ezMath::SmallEpsilon<float>())
  {
    return EZ_FAILURE;
  }

  const float fNear = nearW / nearLength;
  const float fFar = farW / farLength;

  if (ezMath::IsEqual(fNear, fFar, ezMath::SmallEpsilon<float>()))
  {
    return EZ_FAILURE;
  }

  out_fNear = fNear;
  out_fFar = fFar;

  return EZ_SUCCESS;
}

ezPlane ezGraphicsUtils::ComputeInterpolatedFrustumPlane(FrustumPlaneInterpolation direction, float fLerpFactor, const ezMat4& mProjectionMatrix, ezClipSpaceDepthRange::Enum depthRange)
{
  ezVec4 rowA;
  ezVec4 rowB = mProjectionMatrix.GetRow(3);
  const float factorMinus1to1 = (fLerpFactor - 0.5f) * 2.0f; // bring into [-1; +1] range

  switch (direction)
  {
    case FrustumPlaneInterpolation::LeftToRight:
    {
      rowA = mProjectionMatrix.GetRow(0);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::BottomToTop:
    {
      rowA = mProjectionMatrix.GetRow(1);
      rowB *= factorMinus1to1;
      break;
    }

    case FrustumPlaneInterpolation::NearToFar:
      rowA = mProjectionMatrix.GetRow(2);

      if (depthRange == ezClipSpaceDepthRange::ZeroToOne)
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

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum range, ezHandedness::Enum handedness)
{
  const float vw = fViewWidth * 0.5f;
  const float vh = fViewHeight * 0.5f;

  return CreatePerspectiveProjectionMatrix(-vw, vw, -vh, vh, fNearZ, fFarZ, depthRange, range, handedness);
}

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovX(ezAngle fieldOfViewX, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum range, ezHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float xm = ezMath::Min(fNearZ, fFarZ) * ezMath::Tan(fieldOfViewX * 0.5f);
  const float ym = xm / fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrixFromFovY(ezAngle fieldOfViewY, float fAspectRatioWidthDivHeight, float fNearZ, float fFarZ, ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum range, ezHandedness::Enum handedness)
{
  // Taking the minimum allows the function to be used to create
  // inverse z matrices (fNearZ > fFarZ) as well.
  const float ym = ezMath::Min(fNearZ, fFarZ) * ezMath::Tan(fieldOfViewY * 0.5);
  const float xm = ym * fAspectRatioWidthDivHeight;

  return CreatePerspectiveProjectionMatrix(-xm, xm, -ym, ym, fNearZ, fFarZ, depthRange, range, handedness);
}

ezMat4 ezGraphicsUtils::CreateOrthographicProjectionMatrix(float fViewWidth, float fViewHeight, float fNearZ, float fFarZ, ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum range, ezHandedness::Enum handedness)
{
  return CreateOrthographicProjectionMatrix(-fViewWidth * 0.5f, fViewWidth * 0.5f, -fViewHeight * 0.5f, fViewHeight * 0.5f, fNearZ, fFarZ, depthRange, range, handedness);
}

ezMat4 ezGraphicsUtils::CreateOrthographicProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum range, ezHandedness::Enum handedness)
{
  EZ_ASSERT_DEBUG(ezMath::IsFinite(fNearZ) && ezMath::IsFinite(fFarZ), "Infinite plane values are not supported for orthographic projections!");

  ezMat4 res;
  res.SetIdentity();

  if (range == ezClipSpaceYMode::Flipped)
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


  if (depthRange == ezClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    res.Element(2, 2) = -2.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -(fFarZ + fNearZ) * fOneDivFarMinusNear;
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixorthooffcenterrh

    res.Element(2, 2) = -1.0f * fOneDivFarMinusNear;
    res.Element(3, 2) = -fNearZ * fOneDivFarMinusNear;
  }

  if (handedness == ezHandedness::LeftHanded)
  {
    res.SetColumn(2, -res.GetColumn(2));
  }

  return res;
}

ezMat4 ezGraphicsUtils::CreatePerspectiveProjectionMatrix(float fLeft, float fRight, float fBottom, float fTop, float fNearZ, float fFarZ, ezClipSpaceDepthRange::Enum depthRange, ezClipSpaceYMode::Enum range, ezHandedness::Enum handedness)
{
  EZ_ASSERT_DEBUG(ezMath::IsFinite(fNearZ) || ezMath::IsFinite(fFarZ), "fNearZ and fFarZ cannot both be infinite at the same time!");

  ezMat4 res;
  res.SetZero();

  if (range == ezClipSpaceYMode::Flipped)
  {
    ezMath::Swap(fBottom, fTop);
  }

  // Taking the minimum of the two plane values allows
  // this function to also be used to create inverse-z
  // matrices by specifying values of fNearZ > fFarZ.
  // Otherwise the x and y scaling values will be wrong
  // in the final matrix.
  const float fMinPlane = ezMath::Min(fNearZ, fFarZ);
  const float fTwoNearZ = fMinPlane + fMinPlane;
  const float fOneDivRightMinusLeft = 1.0f / (fRight - fLeft);
  const float fOneDivTopMinusBottom = 1.0f / (fTop - fBottom);

  res.Element(0, 0) = fTwoNearZ * fOneDivRightMinusLeft;

  res.Element(1, 1) = fTwoNearZ * fOneDivTopMinusBottom;

  res.Element(2, 0) = (fLeft + fRight) * fOneDivRightMinusLeft;
  res.Element(2, 1) = (fTop + fBottom) * fOneDivTopMinusBottom;
  res.Element(2, 3) = -1.0f;

  // If either fNearZ or fFarZ is infinite, one can derive the resulting z-transformation by using limit math
  // and letting the respective variable approach infinity in the original expressions for P(2, 2) and P(3, 2).
  // The result is that a couple of terms from the original fraction get reduced to 0 by being divided by infinity,
  // which fortunately yields 1) finite and 2) much simpler expressions for P(2, 2) and P(3, 2).
  if (depthRange == ezClipSpaceDepthRange::MinusOneToOne)
  {
    // The OpenGL Way: http://wiki.delphigl.com/index.php/glFrustum
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    // res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f) + 1.f / (1.f - fFarZ / fNearZ);
    // res.Element(3, 2) = 2.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!ezMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 1.f;
      res.Element(3, 2) = 2.f * fFarZ;
    }
    else if (!ezMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.f;
      res.Element(3, 2) = -2.f * fNearZ;
    }
    else
    {
      const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);
      res.Element(2, 2) = (fFarZ + fNearZ) * fOneDivNearMinusFar;
      res.Element(3, 2) = 2 * fFarZ * fNearZ * fOneDivNearMinusFar;
    }
  }
  else
  {
    // The Left-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterlh
    // The Right-Handed Direct3D Way: https://docs.microsoft.com/windows/win32/direct3d9/d3dxmatrixperspectiveoffcenterrh
    // Algebraically reordering the z-row fractions from the above source in a way so infinite fNearZ or fFarZ will zero out
    // instead of producing NaNs due to inf/inf divisions will yield these generalized formulas which could be used instead
    // of the branching below. Insert infinity for either fNearZ or fFarZ to see that these will yield exactly these simplifications:
    // res.Element(2, 2) = 1.f / (fNearZ / fFarZ - 1.f);
    // res.Element(3, 2) = 1.f / (1.f / fFarZ - 1.f / fNearZ);
    if (!ezMath::IsFinite(fNearZ))
    {
      res.Element(2, 2) = 0.f;
      res.Element(3, 2) = fFarZ;
    }
    else if (!ezMath::IsFinite(fFarZ))
    {
      res.Element(2, 2) = -1.f;
      res.Element(3, 2) = -fNearZ;
    }
    else
    {
      const float fOneDivNearMinusFar = 1.0f / (fNearZ - fFarZ);
      res.Element(2, 2) = fFarZ * fOneDivNearMinusFar;
      res.Element(3, 2) = fFarZ * fNearZ * fOneDivNearMinusFar;
    }
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
  vLookDir.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();

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
  vLookDir.NormalizeIfNotZero(ezVec3::MakeAxisX()).IgnoreResult();

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

ezMat4 ezGraphicsUtils::CreateLookAtViewMatrix(const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  const ezMat3 rotation = CreateLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  ezMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(rotation * -vEyePos);
  res.SetRow(3, ezVec4(0, 0, 0, 1));
  return res;
}

ezMat4 ezGraphicsUtils::CreateInverseLookAtViewMatrix(const ezVec3& vEyePos, const ezVec3& vLookAtPos, const ezVec3& vUpDir, ezHandedness::Enum handedness)
{
  const ezMat3 rotation = CreateInverseLookAtViewMatrix(vLookAtPos - vEyePos, vUpDir, handedness);

  ezMat4 res;
  res.SetRotationalPart(rotation);
  res.SetTranslationVector(vEyePos);
  res.SetRow(3, ezVec4(0, 0, 0, 1));
  return res;
}

ezMat4 ezGraphicsUtils::CreateViewMatrix(const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir, const ezVec3& vUpDir, ezHandedness::Enum handedness)
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

ezMat4 ezGraphicsUtils::CreateInverseViewMatrix(const ezVec3& vPosition, const ezVec3& vForwardDir, const ezVec3& vRightDir, const ezVec3& vUpDir, ezHandedness::Enum handedness)
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

void ezGraphicsUtils::DecomposeViewMatrix(ezVec3& ref_vPosition, ezVec3& ref_vForwardDir, ezVec3& ref_vRightDir, ezVec3& ref_vUpDir, const ezMat4& mViewMatrix, ezHandedness::Enum handedness)
{
  const ezMat3 rotation = mViewMatrix.GetRotationalPart();

  if (handedness == ezHandedness::LeftHanded)
  {
    ref_vRightDir = rotation.GetRow(0);
    ref_vUpDir = rotation.GetRow(1);
    ref_vForwardDir = rotation.GetRow(2);
  }
  else
  {
    ref_vRightDir = rotation.GetRow(0);
    ref_vUpDir = rotation.GetRow(1);
    ref_vForwardDir = -rotation.GetRow(2);
  }

  ref_vPosition = rotation.GetTranspose() * -mViewMatrix.GetTranslationVector();
}

ezResult ezGraphicsUtils::ComputeBarycentricCoordinates(ezVec3& out_vCoordinates, const ezVec3& a, const ezVec3& b, const ezVec3& c, const ezVec3& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/49370

  const ezVec3 v0 = b - a;
  const ezVec3 v1 = c - a;
  const ezVec3 v2 = p - a;

  const float d00 = v0.Dot(v0);
  const float d01 = v0.Dot(v1);
  const float d11 = v1.Dot(v1);
  const float d20 = v2.Dot(v0);
  const float d21 = v2.Dot(v1);
  const float denom = d00 * d11 - d01 * d01;

  if (ezMath::IsZero(denom, ezMath::SmallEpsilon<float>()))
    return EZ_FAILURE;

  const float invDenom = 1.0f / denom;

  const float v = (d11 * d20 - d01 * d21) * invDenom;
  const float w = (d00 * d21 - d01 * d20) * invDenom;
  const float u = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return EZ_SUCCESS;
}

ezResult ezGraphicsUtils::ComputeBarycentricCoordinates(ezVec3& out_vCoordinates, const ezVec2& a, const ezVec2& b, const ezVec2& c, const ezVec2& p)
{
  // implementation copied from https://gamedev.stackexchange.com/a/63203

  const ezVec2 v0 = b - a;
  const ezVec2 v1 = c - a;
  const ezVec2 v2 = p - a;

  const float denom = v0.x * v1.y - v1.x * v0.y;

  if (ezMath::IsZero(denom, ezMath::SmallEpsilon<float>()))
    return EZ_FAILURE;

  const float invDenom = 1.0f / denom;
  const float v = (v2.x * v1.y - v1.x * v2.y) * invDenom;
  const float w = (v0.x * v2.y - v2.x * v0.y) * invDenom;
  const float u = 1.0f - v - w;

  out_vCoordinates.Set(u, v, w);

  return EZ_SUCCESS;
}
