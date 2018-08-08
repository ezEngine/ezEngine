#pragma once

#include <RendererCore/Basics.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Transform.h>

class ezSkeleton;

/// \brief The animation pose encapsulates the final transform matrices for each joint in a given skeleton.
/// For each joint there is also a bit flag indicating whether the transform is valid or not. An IK system for example may only
/// generate a couple of valid transforms and will ignore all other joints which are not influenced by the IK system.
class EZ_RENDERERCORE_DLL ezAnimationPose
{
public:
  ezAnimationPose();
  ~ezAnimationPose();

  void Configure(const ezSkeleton& skeleton);

  /// \brief Sets all transforms to the local bind pose of the skeleton.
  /// This is typically used to initialize a skeleton to a default start state.
  void SetToBindPose(const ezSkeleton& skeleton);

  /// \brief Converts the pose from a collection of local transforms to a usable object space transform collection.
  /// This modifies the transforms in place. This is typically the very last modification on a pose before it is sent to the renderer for skinning.
  void CalculateObjectSpaceTransforms(const ezSkeleton& skeleton);

  const ezMat4& GetTransform(ezUInt32 uiIndex) const { return m_Transforms[uiIndex]; }

  ezArrayPtr<const ezMat4> GetAllTransforms() const { return m_Transforms.GetArrayPtr(); }
  bool IsTransformValid(ezUInt32 uiIndex) const { return m_TransformsValid.IsSet(uiIndex); }

  /// \brief Sets the transform for the given index.
  /// This will also set the flag indicating that the transform is valid.
  void SetTransform(ezUInt32 uiIndex, const ezMat4& transform);

  /// \brief Sets the valid flag for a given transform manually.
  void SetTransformValid(ezUInt32 uiIndex, bool bValid);

  /// \brief Helper to set the valid flag of all transforms to a single value.
  void SetValidityOfAllTransforms(bool bValid);

  /// \brief Returns the number of transforms in the pose.
  ezUInt32 GetTransformCount() const { return m_Transforms.GetCount(); }

  /// \brief Helper to skin a position with a single joint (rigid skinning)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinPositionWithSingleJoint(const ezVec3& Position, ezUInt32 uiIndex) const;

  /// \brief Helper to skin a position with four joints (four indices + four weights)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinPositionWithFourJoints(const ezVec3& Position, const ezVec4U32& indices, const ezVec4& weights) const;

  /// \brief Helper to skin a direction with a single joint (rigid skinning)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinDirectionWithSingleJoint(const ezVec3& Direction, ezUInt32 uiIndex) const;

  /// \brief Helper to skin a direction with four joints (four indices + four weights)
  /// Note that this shouldn't be used for "real" skinning - this is just for anchors etc. so they are available
  /// on the CPU side of things.
  ezVec3 SkinDirectionWithFourJoints(const ezVec3& Direction, const ezVec4U32& indices, const ezVec4& weights) const;

private:
  // TODO: would be nicer to use ezTransform or ezShaderTransform for this data

  // use an aligned allocator to make sure this can be uploaded to the GPU
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_Transforms;
  ezDynamicBitfield m_TransformsValid;
};

