#pragma once

inline ezVec3 ezCamera::GetCenterPosition() const
{
  if (m_Mode == ezCameraMode::Stereo)
    return (GetPosition(ezCameraEye::Left) + GetPosition(ezCameraEye::Right)) * 0.5f;
  else
    return GetPosition();
}

inline ezVec3 ezCamera::GetCenterDirForwards() const
{
  if (m_Mode == ezCameraMode::Stereo)
    return (GetDirForwards(ezCameraEye::Left) + GetDirForwards(ezCameraEye::Right)).GetNormalized();
  else
    return GetDirForwards();
}

inline ezVec3 ezCamera::GetCenterDirUp() const
{
  if (m_Mode == ezCameraMode::Stereo)
    return (GetDirUp(ezCameraEye::Left) + GetDirUp(ezCameraEye::Right)).GetNormalized();
  else
    return GetDirUp();
}

inline ezVec3 ezCamera::GetCenterDirRight() const
{
  if (m_Mode == ezCameraMode::Stereo)
    return (GetDirRight(ezCameraEye::Left) + GetDirRight(ezCameraEye::Right)).GetNormalized();
  else
    return GetDirRight();
}

EZ_ALWAYS_INLINE float ezCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

EZ_ALWAYS_INLINE float ezCamera::GetFarPlane() const
{
  return m_fFarPlane;
}

EZ_ALWAYS_INLINE float ezCamera::GetFovOrDim() const
{
  return m_fFovOrDim;
}

EZ_ALWAYS_INLINE ezCameraMode::Enum ezCamera::GetCameraMode() const
{
  return m_Mode;
}

EZ_ALWAYS_INLINE bool ezCamera::IsPerspective() const
{
  return m_Mode == ezCameraMode::PerspectiveFixedFovX || m_Mode == ezCameraMode::PerspectiveFixedFovY ||
         m_Mode == ezCameraMode::Stereo; // All HMD stereo cameras are perspective!
}

EZ_ALWAYS_INLINE bool ezCamera::IsOrthographic() const
{
  return m_Mode == ezCameraMode::OrthoFixedWidth || m_Mode == ezCameraMode::OrthoFixedHeight;
}

EZ_ALWAYS_INLINE bool ezCamera::IsStereoscopic() const
{
  return m_Mode == ezCameraMode::Stereo;
}

EZ_ALWAYS_INLINE float ezCamera::GetExposure() const
{
  return m_fExposure;
}

EZ_ALWAYS_INLINE void ezCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}

EZ_ALWAYS_INLINE const ezMat4& ezCamera::GetViewMatrix(ezCameraEye eye) const
{
  return m_mViewMatrix[static_cast<int>(eye)];
}

