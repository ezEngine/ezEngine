#pragma once

inline ezUInt8 ezFrustum::GetNumPlanes() const
{
  return m_uiUsedPlanes;
}

inline const ezPlane& ezFrustum::GetPlane(ezUInt8 uiPlane) const
{
  EZ_ASSERT_DEV(uiPlane < m_uiUsedPlanes, "Invalid plane index.");

  return m_Planes[uiPlane];
}

inline const ezVec3& ezFrustum::GetPosition() const
{
  return m_vPosition;
}


