#pragma once

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Mat4.h>
#include <RendererCore/Basics.h>

class ezSkeleton;

/// \brief The animation pose encapsulates the final transform matrices for each bone in a given skeleton.
/// For each bone there is also a bit flag indicating whether the transform is valid or not. An IK system for example may only
/// generate a couple of valid transforms and will ignore all other bones which are not influenced by the IK system.
class EZ_RENDERERCORE_DLL ezAnimationPose
{
public:
  inline const ezMat4& GetBoneTransform(ezUInt32 uiBoneIndex) const;
  inline ezArrayPtr<const ezMat4> GetAllBoneTransforms() const;
  inline bool IsBoneTransformValid(ezUInt32 uiBoneIndex) const;

  /// \brief Sets the bone transform (final transform!) for the given bone index.
  /// This will also by default set the bit flag indicating that the bone transform is valid.
  inline void SetBoneTransform(ezUInt32 uiBoneIndex, const ezMat4& BoneTransform);

  /// \brief Sets the valid flag for a given bone index manually.
  /// Note that SetBoneTransform will set the validity flag to true when setting a transform automatically.
  inline void SetBoneTransformValid(ezUInt32 uiBoneIndex, bool bTransformValid);

  /// \brief Helper to set the valid flags of all bone transforms to a single value.
  inline void SetValidityOfAllBoneTransforms(bool bTransformsValid);

  /// \brief Returns the number of bone transforms in the pose.
  inline ezUInt32 GetBoneTransformCount() const;

  /// \brief Helper to skin a position with a single bone (rigid skinning)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinPositionWithSingleBone(const ezVec3& Position, ezUInt32 uiBoneIndex) const;

  /// \brief Helper to skin a position with four bones (four indices + four weights)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinPositionWithFourBones(const ezVec3& Position, const ezVec4U32& BoneIndices, const ezVec4& BoneWeights) const;

  /// \brief Helper to skin a direction with a single bone (rigid skinning)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinDirectionWithSingleBone(const ezVec3& Direction, ezUInt32 uiBoneIndex) const;

  /// \brief Helper to skin a direction with four bones (four indices + four weights)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinDirectionWithFourBones(const ezVec3& Direction, const ezVec4U32& BoneIndices, const ezVec4& BoneWeights) const;

protected:
  friend ezSkeleton;

  // Animation poses belong to specific skeletons, thus only the skeleton can provide new pose objects.
  ezAnimationPose(const ezSkeleton* skeleton);

  // used an aligned allocator to make sure this can be uploaded to the GPU
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_BoneTransforms;
  ezDynamicBitfield m_BoneTransformsValid;
};

#include <RendererCore/AnimationSystem/Implementation/AnimationPose_inl.h>
