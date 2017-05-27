#pragma once

EZ_ALWAYS_INLINE ezVec3 ezCamera::GetPosition() const
{ 
  return m_vPosition; 
}

EZ_ALWAYS_INLINE const ezVec3& ezCamera::GetDirForwards() const
{
  return m_vDirForwards;
}

EZ_ALWAYS_INLINE const ezVec3& ezCamera::GetDirUp() const
{
  return m_vDirUp;
}

EZ_ALWAYS_INLINE const ezVec3& ezCamera::GetDirRight() const
{
  return m_vDirRight;
}

EZ_ALWAYS_INLINE const ezVec3& ezCamera::GetCenterPosition() const
{
  return m_vPosition;
}

EZ_ALWAYS_INLINE const ezVec3& ezCamera::GetCenterDirForwards() const
{
  return m_vDirForwards;
}

EZ_ALWAYS_INLINE const ezVec3& ezCamera::GetCenterDirUp() const
{
  return m_vDirUp;
}

EZ_ALWAYS_INLINE const ezVec3& ezCamera::GetCenterDirRight() const
{
  return m_vDirRight;
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
  return m_Mode == ezCameraMode::PerspectiveFixedFovX || m_Mode == ezCameraMode::PerspectiveFixedFovY;
}

EZ_ALWAYS_INLINE bool ezCamera::IsOrthographic() const
{
  return m_Mode == ezCameraMode::OrthoFixedWidth || m_Mode == ezCameraMode::OrthoFixedHeight;
}

EZ_ALWAYS_INLINE float ezCamera::GetExposure() const
{
  return m_fExposure;
}

EZ_ALWAYS_INLINE void ezCamera::SetExposure(float fExposure)
{
  m_fExposure = fExposure;
}


