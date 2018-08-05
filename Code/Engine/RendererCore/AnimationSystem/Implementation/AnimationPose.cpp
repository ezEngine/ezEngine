#include <PCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

ezAnimationPose::ezAnimationPose(const ezSkeleton* skeleton)
{
  EZ_ASSERT_DEV(skeleton && skeleton->GetBoneCount() > 0, "Animation pose needs a valid skeleton which also has at least one bone!");

  // Allocate storage once for matrices and validity bits
  m_BoneTransforms.SetCountUninitialized(skeleton->GetBoneCount());

  // By default all transforms are invalid.
  m_BoneTransformsValid.SetCount(skeleton->GetBoneCount());
  m_BoneTransformsValid.ClearAllBits();
}

ezVec3 ezAnimationPose::SkinPositionWithSingleBone(const ezVec3& Position, ezUInt32 uiBoneIndex) const
{
  return m_BoneTransforms[uiBoneIndex].TransformPosition(Position);
}

ezVec3 ezAnimationPose::SkinPositionWithFourBones(const ezVec3& Position, const ezVec4U32& BoneIndices, const ezVec4& BoneWeights) const
{
  ezVec3 Pos1 = m_BoneTransforms[BoneIndices.x].TransformPosition(Position);
  ezVec3 Pos2 = m_BoneTransforms[BoneIndices.y].TransformPosition(Position);
  ezVec3 Pos3 = m_BoneTransforms[BoneIndices.z].TransformPosition(Position);
  ezVec3 Pos4 = m_BoneTransforms[BoneIndices.w].TransformPosition(Position);

  return (Pos1 * BoneWeights.x) + (Pos2 * BoneWeights.y) + (Pos3 * BoneWeights.z) + (Pos4 * BoneWeights.w);
}

ezVec3 ezAnimationPose::SkinDirectionWithSingleBone(const ezVec3& Direction, ezUInt32 uiBoneIndex) const
{
  return m_BoneTransforms[uiBoneIndex].TransformDirection(Direction);
}

ezVec3 ezAnimationPose::SkinDirectionWithFourBones(const ezVec3& Direction, const ezVec4U32& BoneIndices, const ezVec4& BoneWeights) const
{
  ezVec3 Dir1 = m_BoneTransforms[BoneIndices.x].TransformDirection(Direction);
  ezVec3 Dir2 = m_BoneTransforms[BoneIndices.y].TransformDirection(Direction);
  ezVec3 Dir3 = m_BoneTransforms[BoneIndices.z].TransformDirection(Direction);
  ezVec3 Dir4 = m_BoneTransforms[BoneIndices.w].TransformDirection(Direction);

  return (Dir1 * BoneWeights.x) + (Dir2 * BoneWeights.y) + (Dir3 * BoneWeights.z) + (Dir4 * BoneWeights.w);
}
