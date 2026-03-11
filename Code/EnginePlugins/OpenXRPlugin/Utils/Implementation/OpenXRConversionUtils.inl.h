
EZ_ALWAYS_INLINE XrPosef ezOpenXRConversionUtils::ConvertTransform(const ezTransform& tr)
{
  XrPosef pose;
  pose.orientation = ConvertOrientation(tr.m_qRotation);
  pose.position = ConvertPosition(tr.m_vPosition);
  return pose;
}

EZ_ALWAYS_INLINE XrQuaternionf ezOpenXRConversionUtils::ConvertOrientation(const ezQuat& q)
{
  return {q.y, q.z, -q.x, -q.w};
}

EZ_ALWAYS_INLINE XrVector3f ezOpenXRConversionUtils::ConvertPosition(const ezVec3& vPos)
{
  return {vPos.y, vPos.z, -vPos.x};
}

EZ_ALWAYS_INLINE ezQuat ezOpenXRConversionUtils::ConvertOrientation(const XrQuaternionf& q)
{
  return {-q.z, q.x, q.y, -q.w};
}

EZ_ALWAYS_INLINE ezVec3 ezOpenXRConversionUtils::ConvertPosition(const XrVector3f& pos)
{
  return {-pos.z, pos.x, pos.y};
}

EZ_ALWAYS_INLINE ezMat4 ezOpenXRConversionUtils::ConvertPoseToMatrix(const XrPosef& pose)
{
  ezMat4 m;
  ezMat3 rot = ConvertOrientation(pose.orientation).GetAsMat3();
  ezVec3 pos = ConvertPosition(pose.position);
  m.SetTransformationMatrix(rot, pos);
  return m;
}
