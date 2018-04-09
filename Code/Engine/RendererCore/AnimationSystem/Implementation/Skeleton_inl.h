
inline const ezMat4& ezSkeleton::Bone::GetBoneTransform() const
{
	return m_BoneTransform;
}

inline const ezMat4& ezSkeleton::Bone::GetInverseBindPoseTransform() const
{
  return m_InverseBindPoseTransform;
}

inline ezUInt32 ezSkeleton::Bone::GetParentIndex() const
{
	return m_uiParentIndex;
}

inline bool ezSkeleton::Bone::IsRootBone() const
{
	return m_uiParentIndex == 0xFFFFFFFFu;
}

inline const ezHashedString& ezSkeleton::Bone::GetName() const
{
	return m_sName;
}


inline const ezSkeleton::Bone& ezSkeleton::GetBone( ezUInt32 uiBoneIndex ) const
{
	return m_Bones[uiBoneIndex];
}
