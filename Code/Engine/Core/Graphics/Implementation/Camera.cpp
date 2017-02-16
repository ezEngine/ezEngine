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

  m_vPosition.SetZero();
  m_vDirForwards = ezVec3(1.0f, 0.0f, 0.0f);
  m_vDirUp = ezVec3(0.0f, 0.0f, 1.0f);
  m_vDirRight = ezVec3(0.0f, 1.0f, 0.0f);

  m_fExposure = 1.0f;

  m_uiSettingsModificationCounter = 0;
  m_uiOrientationModificationCounter = 0;
}

ezAngle ezCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::PerspectiveFixedFovX)
    return ezAngle::Degree(m_fFovOrDim);

  if (m_Mode == ezCameraMode::PerspectiveFixedFovY)
    return ezAngle::Degree(m_fFovOrDim) * fAspectRatioWidthDivHeight;

  EZ_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return ezAngle();
}

ezAngle ezCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == ezCameraMode::PerspectiveFixedFovX)
    return ezAngle::Degree(m_fFovOrDim) / fAspectRatioWidthDivHeight;

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

  CameraSettingsChanged();
}

void ezCamera::LookAt(const ezVec3& vCameraPos, const ezVec3& vTargetPos, const ezVec3& vUp)
{
  m_vPosition = vCameraPos;

  ezMat3 mTmp; mTmp.SetLookInDirectionMatrix(vTargetPos - vCameraPos, vUp);
  m_vDirRight = mTmp.GetRow(0);
  m_vDirUp = mTmp.GetRow(1);
  m_vDirForwards = mTmp.GetRow(2);

  CameraOrientationChanged(true, true);
}

void ezCamera::SetFromMatrix(const ezMat4& mLookAtMatrix)
{
  m_vPosition    = -(mLookAtMatrix.GetRotationalPart().GetTranspose() * mLookAtMatrix.GetTranslationVector());
  m_vDirRight    =  mLookAtMatrix.GetRow(0).GetAsVec3();
  m_vDirUp       =  mLookAtMatrix.GetRow(1).GetAsVec3();
  m_vDirForwards =  mLookAtMatrix.GetRow(2).GetAsVec3();

  CameraOrientationChanged(true, true);
}

void ezCamera::GetViewMatrix(ezMat4& out_viewMatrix) const
{
  out_viewMatrix.SetLookAtMatrix(m_vPosition, m_vPosition + m_vDirForwards, m_vDirUp);
}

void ezCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, ezMat4& out_projectionMatrix, ezProjectionDepthRange::Enum depthRange) const
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

  diff += m_vDirForwards * fForward;
  diff += m_vDirRight * fRight;
  diff += m_vDirUp * fUp;

  m_vPosition += diff;

  CameraOrientationChanged(true, false);

  return diff;
}

void ezCamera::MoveGlobally(const ezVec3& vMove)
{
  m_vPosition += vMove;

  CameraOrientationChanged(true, false);
}

void ezCamera::ClampRotationAngles(bool bLocalSpace, ezAngle& X, ezAngle& Y, ezAngle& Z)
{
  if (bLocalSpace)
  {
    if (Y.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning

      const float fDot = m_vDirForwards.Dot(ezVec3(0, 0, 1));
      ezAngle fCurAngle = ezMath::ACos(fDot) - ezAngle::Degree(90.0f);
      ezAngle fNewAngle = fCurAngle + Y;

      ezAngle fAllowedAngle = ezMath::Clamp(fNewAngle, ezAngle::Degree(-85.0f), ezAngle::Degree(85.0f));

      Y = fAllowedAngle - fCurAngle;
    }
  }
}

void ezCamera::RotateLocally (ezAngle X, ezAngle Y, ezAngle Z)
{
  ClampRotationAngles(true, X, Y, Z);

  if (X.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(m_vDirForwards, X);

    m_vDirUp = m * m_vDirUp;
    m_vDirRight = m * m_vDirRight;
  }

  if (Y.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(m_vDirRight, Y);

    m_vDirUp = m * m_vDirUp;
    m_vDirForwards = m * m_vDirForwards;
  }

  if (Z.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(m_vDirUp, Z);

    m_vDirRight = m * m_vDirRight;
    m_vDirForwards = m * m_vDirForwards;
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

    m_vDirRight    = m * m_vDirRight;
    m_vDirUp       = m * m_vDirUp;
    m_vDirForwards = m * m_vDirForwards;
  }

  if (Y.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixY(Y);

    m_vDirRight    = m * m_vDirRight;
    m_vDirUp       = m * m_vDirUp;
    m_vDirForwards = m * m_vDirForwards;
  }

  if (Z.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrixZ(Z);

    m_vDirRight    = m * m_vDirRight;
    m_vDirUp       = m * m_vDirUp;
    m_vDirForwards = m * m_vDirForwards;
  }

  CameraOrientationChanged(false, true);
}


EZ_STATICLINK_FILE(Core, Core_Graphics_Implementation_Camera);

