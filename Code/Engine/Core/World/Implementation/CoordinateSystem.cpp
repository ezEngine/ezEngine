#include <Core/CorePCH.h>

#include <Core/World/CoordinateSystem.h>


ezCoordinateSystemConversion::ezCoordinateSystemConversion()
{
  m_mSourceToTarget.SetIdentity();
  m_mTargetToSource.SetIdentity();
}

void ezCoordinateSystemConversion::SetConversion(const ezCoordinateSystem& source, const ezCoordinateSystem& target)
{
  float fSourceScale = source.m_vForwardDir.GetLengthSquared();
  EZ_ASSERT_DEV(ezMath::IsEqual(fSourceScale, source.m_vRightDir.GetLengthSquared(), ezMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  EZ_ASSERT_DEV(ezMath::IsEqual(fSourceScale, source.m_vUpDir.GetLengthSquared(), ezMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  ezMat3 mSourceFromId;
  mSourceFromId.SetColumn(0, source.m_vRightDir);
  mSourceFromId.SetColumn(1, source.m_vUpDir);
  mSourceFromId.SetColumn(2, source.m_vForwardDir);

  float fTargetScale = target.m_vForwardDir.GetLengthSquared();
  EZ_ASSERT_DEV(ezMath::IsEqual(fTargetScale, target.m_vRightDir.GetLengthSquared(), ezMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  EZ_ASSERT_DEV(ezMath::IsEqual(fTargetScale, target.m_vUpDir.GetLengthSquared(), ezMath::DefaultEpsilon<float>()),
    "Only uniformly scaled coordinate systems are supported");
  ezMat3 mTargetFromId;
  mTargetFromId.SetColumn(0, target.m_vRightDir);
  mTargetFromId.SetColumn(1, target.m_vUpDir);
  mTargetFromId.SetColumn(2, target.m_vForwardDir);

  m_mSourceToTarget = mTargetFromId * mSourceFromId.GetInverse();
  m_mSourceToTarget.SetColumn(0, m_mSourceToTarget.GetColumn(0).GetNormalized());
  m_mSourceToTarget.SetColumn(1, m_mSourceToTarget.GetColumn(1).GetNormalized());
  m_mSourceToTarget.SetColumn(2, m_mSourceToTarget.GetColumn(2).GetNormalized());

  m_fWindingSwap = m_mSourceToTarget.GetDeterminant() < 0 ? -1.0f : 1.0f;
  m_fSourceToTargetScale = 1.0f / ezMath::Sqrt(fSourceScale) * ezMath::Sqrt(fTargetScale);
  m_mTargetToSource = m_mSourceToTarget.GetInverse();
  m_fTargetToSourceScale = 1.0f / m_fSourceToTargetScale;
}

ezVec3 ezCoordinateSystemConversion::ConvertSourcePosition(const ezVec3& vPos) const
{
  return m_mSourceToTarget * vPos * m_fSourceToTargetScale;
}

ezQuat ezCoordinateSystemConversion::ConvertSourceRotation(const ezQuat& qOrientation) const
{
  ezVec3 axis = m_mSourceToTarget * qOrientation.GetVectorPart();
  ezQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float ezCoordinateSystemConversion::ConvertSourceLength(float fLength) const
{
  return fLength * m_fSourceToTargetScale;
}

ezVec3 ezCoordinateSystemConversion::ConvertTargetPosition(const ezVec3& vPos) const
{
  return m_mTargetToSource * vPos * m_fTargetToSourceScale;
}

ezQuat ezCoordinateSystemConversion::ConvertTargetRotation(const ezQuat& qOrientation) const
{
  ezVec3 axis = m_mTargetToSource * qOrientation.GetVectorPart();
  ezQuat rr(axis.x, axis.y, axis.z, qOrientation.w * m_fWindingSwap);
  return rr;
}

float ezCoordinateSystemConversion::ConvertTargetLength(float fLength) const
{
  return fLength * m_fTargetToSourceScale;
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_CoordinateSystem);
