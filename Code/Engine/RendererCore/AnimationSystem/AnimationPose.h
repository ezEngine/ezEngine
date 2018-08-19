#pragma once

#include <RendererCore/AnimationSystem/Declarations.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Transform.h>

class ezSkeleton;
class ezDebugRendererContext;

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
  void SetToBindPoseInLocalSpace(const ezSkeleton& skeleton);

  /// \brief Converts each joint from local space to object space, ie. it concatenates parent transforms and bakes them into each joint.
  /// 
  /// The result is a pose that can be used for instance for visualizing the skeleton by drawing lines from each joint position to
  /// the parent and child joints.
  /// It is, however, not (yet) suitable for actual GPU skinning, as that happens in a different space.
  void ConvertFromLocalSpaceToObjectSpace(const ezSkeleton& skeleton);

  /// \brief Converts each joint from the hierarchical object space position to the final skinning space, which is used to modify a mesh.
  /// 
  /// This is typically the very last operation done on a pose before it is sent to the GPU for skinning.
  void ConvertFromObjectSpaceToSkinningSpace(const ezSkeleton& skeleton);

  const ezMat4& GetTransform(ezUInt16 uiJointIndex) const { return m_Transforms[uiJointIndex]; }

  ezArrayPtr<const ezMat4> GetAllTransforms() const { return m_Transforms.GetArrayPtr(); }
  bool IsTransformValid(ezUInt16 uiIndex) const { return m_TransformsValid.IsSet(uiIndex); }

  /// \brief Sets the transform for the given index.
  /// This will also set the flag indicating that the transform is valid.
  void SetTransform(ezUInt16 uiIndex, const ezMat4& transform);

  /// \brief Sets the valid flag for a given transform manually.
  void SetTransformValid(ezUInt16 uiIndex, bool bValid);

  /// \brief Helper to set the valid flag of all transforms to a single value.
  void SetValidityOfAllTransforms(bool bValid);

  /// \brief Returns the number of transforms in the pose.
  ezUInt16 GetTransformCount() const { return m_Transforms.GetCount(); }

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

  /// \brief Renders a debug visualization of this pose. \a objectTransform is used to position, rotate and scale the stick-figure as needed.
  ///
  /// Object scale is, however, only partially used, joint indicators are always sized according to the distance to the parent joints (after scaling).
  void VisualizePose(const ezDebugRendererContext& context, const ezSkeleton& skeleton, const ezTransform& objectTransform, ezUInt16 uiStartJoint = ezInvalidJointIndex) const;

private:
  // TODO: would be nicer to use ezTransform or ezShaderTransform for this data

  // use an aligned allocator to make sure this can be uploaded to the GPU
  ezDynamicArray<ezMat4, ezAlignedAllocatorWrapper> m_Transforms;
  ezDynamicBitfield m_TransformsValid;
};

