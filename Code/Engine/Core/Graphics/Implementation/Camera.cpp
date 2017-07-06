#include <PCH.h>
#include <Core/Graphics/Camera.h>

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezCameraMode, 1)
  EZ_ENUM_CONSTANT(ezCameraMode::PerspectiveFixedFovX),
  EZ_ENUM_CONSTANT(ezCameraMode::PerspectiveFixedFovY),
  EZ_ENUM_CONSTANT(ezCameraMode::OrthoFixedWidth),
  EZ_ENUM_CONSTANT(ezCameraMode::OrthoFixedHeight),
EZ_END_STATIC_REFLECTED_ENUM();

ezCamera::ezCamera()
{
  m_fNearPlane = 0.1f;
  m_fFarPlane = 1000.0f;
  m_Mode = ezCameraMode::None;
  m_fFovOrDim = 90.0f;

  m_fExposure = 1.0f;

  m_mViewMatrix[0].SetIdentity();
  m_mViewMatrix[1].SetIdentity();
  m_mStereoProjectionMatrix[0].SetIdentity();
  m_mStereoProjectionMatrix[1].SetIdentity();

  m_uiSettingsModificationCounter = 0;
  m_uiOrientationModificationCounter = 0;
}

ezAngle ezCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::PerspectiveFixedFovX)
    return ezAngle::Degree(m_fFovOrDim);

  if (m_Mode == ezCameraMode::PerspectiveFixedFovY)
    return ezMath::ATan(ezMath::Tan(ezAngle::Degree(m_fFovOrDim) * 0.5f) * fAspectRatioWidthDivHeight) * 2.0f;

  EZ_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return ezAngle();
}

ezAngle ezCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::PerspectiveFixedFovX)
    return ezMath::ATan(ezMath::Tan(ezAngle::Degree(m_fFovOrDim) * 0.5f) / fAspectRatioWidthDivHeight) * 2.0f;

  if (m_Mode == ezCameraMode::PerspectiveFixedFovY)
    return ezAngle::Degree(m_fFovOrDim);

  EZ_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return ezAngle();
}


float ezCamera::GetDimensionX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::OrthoFixedWidth)
    return m_fFovOrDim;

  if (m_Mode == ezCameraMode::OrthoFixedHeight)
    return m_fFovOrDim * fAspectRatioWidthDivHeight;

  EZ_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}


float ezCamera::GetDimensionY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::OrthoFixedWidth)
    return m_fFovOrDim / fAspectRatioWidthDivHeight;

  if (m_Mode == ezCameraMode::OrthoFixedHeight)
    return m_fFovOrDim;

  EZ_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
  return 0;
}

void ezCamera::SetCameraMode(ezCameraMode::Enum Mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == Mode &&
    m_fFovOrDim == fFovOrDim &&
    m_fNearPlane == fNearPlane &&
    m_fFarPlane == fFarPlane)
  {
    return;
  }

  m_Mode = Mode;
  m_fFovOrDim = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane = fFarPlane;

  m_fAspectOfPrecomputedStereoProjection = -1.0f;

  CameraSettingsChanged();
}

void ezCamera::SetStereoProjection(const ezMat4& mProjectionLeftEye, const ezMat4& mProjectionRightEye, float fAspectRatioWidthDivHeight)
{
  m_mStereoProjectionMatrix[static_cast<int>(ezCameraEye::Left)] = mProjectionLeftEye;
  m_mStereoProjectionMatrix[static_cast<int>(ezCameraEye::Right)] = mProjectionRightEye;
  m_fAspectOfPrecomputedStereoProjection = fAspectRatioWidthDivHeight;

  CameraSettingsChanged();
}

void ezCamera::LookAt(const ezVec3& vCameraPos, const ezVec3& vTargetPos, const ezVec3& vUp)
{
  if (m_Mode == ezCameraMode::Stereo)
  {
    EZ_REPORT_FAILURE("You cannot get the camera dimensions when it is not an orthographic camera.");
    return;
  }

  m_mViewMatrix[0].SetLookAtMatrix(vCameraPos, vTargetPos, vUp);
  m_mViewMatrix[1] = m_mViewMatrix[0];

  CameraOrientationChanged(true, true);
}

void ezCamera::SetViewMatrix(const ezMat4& mLookAtMatrix, ezCameraEye eye)
{
  const int iEyeIdx = static_cast<int>(eye);

  m_mViewMatrix[iEyeIdx] = mLookAtMatrix;

  if (m_Mode != ezCameraMode::Stereo)
    m_mViewMatrix[1 - iEyeIdx] = mLookAtMatrix;
  
  CameraOrientationChanged(true, true);
}

void ezCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, ezMat4& out_projectionMatrix, ezCameraEye eye, ezProjectionDepthRange::Enum depthRange) const
{
  if (m_Mode == ezCameraMode::PerspectiveFixedFovX)
  {
    out_projectionMatrix.SetPerspectiveProjectionMatrixFromFovX(ezAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange);
  }
  else if (m_Mode == ezCameraMode::PerspectiveFixedFovY)
  {
    out_projectionMatrix.SetPerspectiveProjectionMatrixFromFovY(ezAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange);
  }
  else if (m_Mode == ezCameraMode::OrthoFixedWidth)
  {
    out_projectionMatrix.SetOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange);
  }
  else if (m_Mode == ezCameraMode::OrthoFixedHeight)
  {
    out_projectionMatrix.SetOrthographicProjectionMatrix(m_fFovOrDim * fAspectRatioWidthDivHeight, m_fFovOrDim, m_fNearPlane, m_fFarPlane, depthRange);
  }
  else if (m_Mode == ezCameraMode::Stereo)
  {
    if (ezMath::IsEqual(m_fAspectOfPrecomputedStereoProjection, fAspectRatioWidthDivHeight, ezMath::BasicType<float>::LargeEpsilon()))
      out_projectionMatrix = m_mStereoProjectionMatrix[static_cast<int>(eye)];
    else
    {
      // Evade to FixedFovY
      out_projectionMatrix.SetPerspectiveProjectionMatrixFromFovY(ezAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange);
    }
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid Camera Mode.");
  }
}

void ezCamera::CameraSettingsChanged()
{
  EZ_ASSERT_DEV(m_Mode != ezCameraMode::None, "Invalid Camera Mode.");
  EZ_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  EZ_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

ezVec3 ezCamera::MoveLocally(float fForward, float fRight, float fUp)
{
  ezVec3 diff(0.0f);

  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() + ezVec3(fRight, fUp, fForward));
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());

  CameraOrientationChanged(true, false);

  return diff;
}

void ezCamera::MoveGlobally(const ezVec3& vMove)
{
  m_mViewMatrix[0].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector() + m_mViewMatrix[0].GetRotationalPart() * vMove);
  m_mViewMatrix[1].SetTranslationVector(m_mViewMatrix[0].GetTranslationVector());


  CameraOrientationChanged(true, false);
}

void ezCamera::ClampRotationAngles(bool bLocalSpace, ezAngle& X, ezAngle& Y, ezAngle& Z)
{
  if (bLocalSpace)
  {
    if (Y.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float fDot = GetDirForwards().Dot(ezVec3(0, 0, 1));
      ezAngle fCurAngle = ezMath::ACos(fDot) - ezAngle::Degree(90.0f);
      ezAngle fNewAngle = fCurAngle + Y;

      ezAngle fAllowedAngle = ezMath::Clamp(fNewAngle, ezAngle::Degree(-85.0f), ezAngle::Degree(85.0f));

      Y = fAllowedAngle - fCurAngle;
    }
  }
}

void ezCamera::SetDirForwards(const ezVec3& vDirForwards)
{
  m_mViewMatrix[1].Element(0, 2) = m_mViewMatrix[0].Element(0, 2) = vDirForwards.x;
  m_mViewMatrix[1].Element(1, 2) = m_mViewMatrix[0].Element(1, 2) = vDirForwards.y;
  m_mViewMatrix[1].Element(2, 2) = m_mViewMatrix[0].Element(2, 2) = vDirForwards.z;
}

void ezCamera::SetDirRight(const ezVec3& vDirRight)
{
  m_mViewMatrix[1].Element(0, 1) = m_mViewMatrix[0].Element(0, 1) = vDirRight.x;
  m_mViewMatrix[1].Element(1, 1) = m_mViewMatrix[0].Element(1, 1) = vDirRight.y;
  m_mViewMatrix[1].Element(2, 1) = m_mViewMatrix[0].Element(2, 1) = vDirRight.z;
}

void ezCamera::SetDirUp(const ezVec3& vDirUp)
{
  m_mViewMatrix[1].Element(0, 0) = m_mViewMatrix[0].Element(0, 0) = vDirUp.x;
  m_mViewMatrix[1].Element(1, 0) = m_mViewMatrix[0].Element(1, 0) = vDirUp.y;
  m_mViewMatrix[1].Element(2, 0) = m_mViewMatrix[0].Element(2, 0) = vDirUp.z;
}

void ezCamera::RotateLocally (ezAngle X, ezAngle Y, ezAngle Z)
{
  ClampRotationAngles(true, X, Y, Z);

  ezVec3 vDirForwards = GetDirForwards();
  ezVec3 vDirUp = GetDirUp();
  ezVec3 vDirRight = GetDirRight();

  if (X.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(vDirForwards, X);

    SetDirUp(m * vDirUp);
    SetDirRight(m * vDirRight);
  }

  if (Y.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(vDirRight, Y);

    SetDirUp(m * vDirUp);
    SetDirForwards(m * vDirForwards);
  }

  if (Z.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(vDirUp, Z);

    SetDirRight(m * vDirRight);
    SetDirForwards(m * vDirForwards);
  }

  CameraOrientationChanged(false, true);
}

void ezCamera::RotateGlobally(ezAngle X, ezAngle Y, ezAngle Z)
{
  ClampRotationAngles(false, X, Y, Z);

  if (X.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixX(X);

    SetDirRight(m * GetDirRight());
    SetDirUp(m * GetDirUp());
    SetDirForwards(m * GetDirForwards());
  }

  if (Y.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixY(Y);

    SetDirRight(m * GetDirRight());
    SetDirUp(m * GetDirUp());
    SetDirForwards(m * GetDirForwards());
  }

  if (Z.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixZ(Z);

    SetDirRight(m * GetDirRight());
    SetDirUp(m * GetDirUp());
    SetDirForwards(m * GetDirForwards());
  }

  CameraOrientationChanged(false, true);
}


EZ_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);

