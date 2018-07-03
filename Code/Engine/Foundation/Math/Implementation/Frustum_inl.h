#pragma once

EZ_ALWAYS_INLINE ezUInt8 ezFrustum::GetNumPlanes() const
{
  return m_uiUsedPlanes;
}

EZ_ALWAYS_INLINE const ezPlane& ezFrustum::GetPlane(ezUInt8 uiPlane) const
{
  EZ_ASSERT_DEBUG(uiPlane < m_uiUsedPlanes, "Invalid plane index.");

  return m_Planes[uiPlane];
}

EZ_ALWAYS_INLINE const ezVec3& ezFrustum::GetPosition() const
{
  return m_vPosition;
}
