#include <CoreUtils/PCH.h>
#include <CoreUtils/Graphics/Camera.h>

ezCamera::ezCamera()
{
  m_Mode = None;
  m_uiSettingsModificationCounter = 0;
  m_uiOrientationModificationCounter = 0;
}

ezAngle ezCamera::GetFovX(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == PerspectiveFixedFovX)
    return ezAngle::Degree(m_fFovOrDim);

  if (m_Mode == PerspectiveFixedFovY)
    return ezAngle::Degree(m_fFovOrDim) * fAspectRatioWidthDivHeight;

  EZ_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return ezAngle();
}

ezAngle ezCamera::GetFovY(float fAspectRatioWidthDivHeight) const
{
  if (m_Mode == PerspectiveFixedFovX)
    return ezAngle::Degree(m_fFovOrDim) / fAspectRatioWidthDivHeight;

  if (m_Mode == PerspectiveFixedFovY)
    return ezAngle::Degree(m_fFovOrDim);

  EZ_REPORT_FAILURE("You cannot get the camera FOV when it is not a perspective camera.");
  return ezAngle();
}

void ezCamera::SetCameraMode(CameraMode Mode, float fFovOrDim, float fNearPlane, float fFarPlane)
{
  // early out if no change
  if (m_Mode == Mode &&
      m_fFovOrDim == fFovOrDim &&
      m_fNearPlane == fNearPlane &&
      m_fFarPlane == fFarPlane)
      return;

  m_Mode = Mode;
  m_fFovOrDim = fFovOrDim;
  m_fNearPlane = fNearPlane;
  m_fFarPlane = fFarPlane;

  CameraSettingsChanged();
}

void ezCamera::LookAt(const ezVec3& vCameraPos, const ezVec3& vTargetPos, const ezVec3& vUp)
{
  m_vPosition = vCameraPos;
  m_vDirForwards = (vTargetPos - vCameraPos).GetNormalized();
  m_vDirRight = m_vDirForwards.Cross(vUp).GetNormalized();
  m_vDirUp = m_vDirRight.Cross(m_vDirForwards).GetNormalized();

  CameraOrientationChanged(true, true);
}

void ezCamera::SetFromMatrix(const ezMat4& mLookAtMatrix)
{
  m_vPosition    = -mLookAtMatrix.GetTranslationVector();
  m_vDirRight    =  mLookAtMatrix.GetRow(0).GetAsVec3();
  m_vDirUp       =  mLookAtMatrix.GetRow(1).GetAsVec3();
  m_vDirForwards = -mLookAtMatrix.GetRow(2).GetAsVec3();

  CameraOrientationChanged(true, true);
}

void ezCamera::GetViewMatrix(ezMat4& out_viewMatrix) const
{
  out_viewMatrix.SetLookAtMatrix(m_vPosition, m_vPosition + m_vDirForwards, m_vDirUp);
}

void ezCamera::GetProjectionMatrix(float fAspectRatioWidthDivHeight, ezMat4& out_projectionMatrix, ezProjectionDepthRange::Enum depthRange) const
{
  if (m_Mode == PerspectiveFixedFovX)
  {
    out_projectionMatrix.SetPerspectiveProjectionMatrixFromFovX(ezAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange);
  }
  else if (m_Mode == PerspectiveFixedFovY)
  {
    out_projectionMatrix.SetPerspectiveProjectionMatrixFromFovY(ezAngle::Degree(m_fFovOrDim), fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange);
  }
  else if (m_Mode == OrthoFixedWidth)
  {
    out_projectionMatrix.SetOrthographicProjectionMatrix(m_fFovOrDim, m_fFovOrDim / fAspectRatioWidthDivHeight, m_fNearPlane, m_fFarPlane, depthRange);
  }
  else if (m_Mode == OrthoFixedHeight)
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
  EZ_ASSERT_DEV(m_Mode != None, "Invalid Camera Mode.");
  EZ_ASSERT_DEV(m_fNearPlane < m_fFarPlane, "Near and Far Plane are invalid.");
  EZ_ASSERT_DEV(m_fFovOrDim > 0.0f, "FOV or Camera Dimension is invalid.");

  ++m_uiSettingsModificationCounter;
}

void ezCamera::MoveLocally (const ezVec3& vMove)
{
  m_vPosition += vMove.x * m_vDirRight;
  m_vPosition += vMove.y * m_vDirUp;
  m_vPosition -= vMove.z * m_vDirForwards;

  CameraOrientationChanged(true, false);
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
    if (X.GetRadian() != 0.0f)
    {
      // Limit how much the camera can look up and down, to prevent it from overturning 

      const float fDot = m_vDirForwards.Dot(ezVec3(0, 1, 0));
      ezAngle fCurAngle = ezAngle::Degree(90.0f) - ezMath::ACos(fDot);
      ezAngle fNewAngle = fCurAngle + X;

      ezAngle fAllowedAngle = ezMath::Clamp(fNewAngle, ezAngle::Degree(-85.0f), ezAngle::Degree(85.0f));

      X = fAllowedAngle - fCurAngle;
    }
  }
}

void ezCamera::RotateLocally (ezAngle X, ezAngle Y, ezAngle Z)
{
  ClampRotationAngles(true, X, Y, Z);

  if (X.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(m_vDirRight, X);

    m_vDirUp = m * m_vDirUp;
    m_vDirForwards = m * m_vDirForwards;
  }

  if (Y.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(m_vDirUp, Y);

    m_vDirRight = m * m_vDirRight;
    m_vDirForwards = m * m_vDirForwards;
  }

  if (Z.GetRadian() != 0.0f)
  {
    ezMat3 m;
    m.SetRotationMatrix(m_vDirForwards, Z);

    m_vDirUp = m * m_vDirUp;
    m_vDirRight = m * m_vDirRight;
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





EZ_STATICLINK_FILE(CoreUtils, CoreUtils_Graphics_Implementation_Camera);

