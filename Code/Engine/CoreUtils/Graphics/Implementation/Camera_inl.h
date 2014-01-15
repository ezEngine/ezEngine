#pragma once

inline ezVec3 ezCamera::GetPosition() const
{ 
  return m_vPosition; 
}

inline const ezVec3& ezCamera::GetDirForwards() const
{
  return m_vDirForwards;
}

inline const ezVec3& ezCamera::GetDirUp() const
{
  return m_vDirUp;
}

inline const ezVec3& ezCamera::GetDirRight() const
{
  return m_vDirRight;
}

inline const ezVec3& ezCamera::GetCenterPosition() const
{
  return m_vPosition;
}

inline const ezVec3& ezCamera::GetCenterDirForwards() const
{
  return m_vDirForwards;
}

inline const ezVec3& ezCamera::GetCenterDirUp() const
{
  return m_vDirUp;
}

inline const ezVec3& ezCamera::GetCenterDirRight() const
{
  return m_vDirRight;
}

inline float ezCamera::GetNearPlane() const
{
  return m_fNearPlane;
}

inline float ezCamera::GetFarPlane() const
{
  return m_fFarPlane;
}




