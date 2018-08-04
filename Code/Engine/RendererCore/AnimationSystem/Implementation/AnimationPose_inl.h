
inline const ezMat4& ezAnimationPose::GetBoneTransform(ezUInt32 uiBoneIndex) const
{
  return m_BoneTransforms[uiBoneIndex];
}

inline bool ezAnimationPose::IsBoneTransformValid(ezUInt32 uiBoneIndex) const
{
  return m_BoneTransformsValid.IsSet(uiBoneIndex);
}

inline void ezAnimationPose::SetBoneTransform(ezUInt32 uiBoneIndex, const ezMat4& BoneTransform)
{
  m_BoneTransforms[uiBoneIndex] = BoneTransform;
  m_BoneTransformsValid.SetBit(uiBoneIndex);
}

inline void ezAnimationPose::SetBoneTransformValid(ezUInt32 uiBoneIndex, bool bTransformValid)
{
  if (bTransformValid)
  {
    m_BoneTransformsValid.SetBit(uiBoneIndex);
  }
  else
  {
    m_BoneTransformsValid.ClearBit(uiBoneIndex);
  }
}

inline void ezAnimationPose::SetValidityOfAllBoneTransforms(bool bTransformsValid)
{
  if (bTransformsValid)
  {
    m_BoneTransformsValid.SetAllBits();
  }
  else
  {
    m_BoneTransformsValid.ClearAllBits();
  }
}

inline ezUInt32 ezAnimationPose::GetBoneTransformCount() const
{
  return m_BoneTransforms.GetCount();
}
