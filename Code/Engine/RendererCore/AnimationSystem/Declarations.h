#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Communication/Message.h>
#include <RendererCore/RendererCoreDLL.h>
#include <ozz/base/maths/soa_transform.h>

class ezSkeleton;
class ezAnimationPose;
struct ezSkeletonResourceDescriptor;
class ezEditableSkeletonJoint;
struct ezAnimationClipResourceDescriptor;
class ezAnimPoseGenerator;
class ezGameObject;

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

#define ezInvalidJointIndex static_cast<ezUInt16>(0xFFFFu)

namespace ozz::animation
{
  class Skeleton;
}

/// \brief What shape is used to approximate a bone's geometry
struct ezSkeletonJointGeometryType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,
    ConvexMesh, ///< A convex mesh is extracted from the mesh file.
    CapsuleSideways,

    Default = None
  };
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose is being prepared.
///
/// The pose matrices are still in local space and in the ozz internal structure-of-arrays format.
/// At this point individual bones can still be modified, to propagate the effect to the child bones.
struct EZ_RENDERERCORE_DLL ezMsgAnimationPosePreparing : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationPosePreparing, ezMessage);

  const ezSkeleton* m_pSkeleton = nullptr;
  ezArrayPtr<ozz::math::SoaTransform> m_LocalTransforms;
};

/// \brief Sent to objects when a parent component is generating an animation pose, to inject additional pose commands, for instance to apply inverse kinematics (IK).
///
/// The message contains the ezAnimPoseGenerator that is currently being built.
/// Usually it has already been executed once and generated a pose (in model space), which can be queried to build upon.
/// Additional commands can then be added to modify the pose.
/// This is mainly meant for inverse kinematics use cases.
struct EZ_RENDERERCORE_DLL ezMsgAnimationPoseGeneration : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationPoseGeneration, ezMessage);

  ezAnimPoseGenerator* m_pGenerator = nullptr;
};

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct EZ_RENDERERCORE_DLL ezMsgAnimationPoseUpdated : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationPoseUpdated, ezMessage);

  static void ComputeFullBoneTransform(const ezMat4& mRootTransform, const ezMat4& mModelTransform, ezMat4& ref_mFullTransform, ezQuat& ref_qRotationOnly);
  void ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& ref_mFullTransform) const;
  void ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& ref_mFullTransform, ezQuat& ref_qRotationOnly) const;

  const ezTransform* m_pRootTransform = nullptr;
  const ezSkeleton* m_pSkeleton = nullptr;
  ezArrayPtr<const ezMat4> m_ModelTransforms;
  bool m_bContinueAnimating = true;
};

/// \brief Used by components that do rope simulation and rendering.
///
/// The rope simulation component sends this message to components attached to the same game object,
/// every time there is a new rope pose. There is no skeleton information, since all joints/bones are
/// connected as one long string.
///
/// For a rope with N segments, N+1 poses are sent. The last pose may use the same rotation as the one before.
struct EZ_RENDERERCORE_DLL ezMsgRopePoseUpdated : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRopePoseUpdated, ezMessage);

  ezArrayPtr<const ezTransform> m_LinkTransforms;
};

/// \brief The animated mesh component listens to this message and 'answers' by filling out the skeleton resource handle.
///
/// This can be used by components that require a skeleton, to ask the nearby components to provide it to them.
struct EZ_RENDERERCORE_DLL ezMsgQueryAnimationSkeleton : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgQueryAnimationSkeleton, ezMessage);

  ezSkeletonResourceHandle m_hSkeleton;
};

/// \brief This message is sent when animation root motion data is available.
///
/// Listening components can use this to move a character.
struct EZ_RENDERERCORE_DLL ezMsgApplyRootMotion : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgApplyRootMotion, ezMessage);

  ezVec3 m_vTranslation;
  ezAngle m_RotationX;
  ezAngle m_RotationY;
  ezAngle m_RotationZ;
};

/// \brief Queries the local transforms of each bone in an object with a skeleton
///
/// Used to retrieve the pose of a ragdoll after simulation.
struct EZ_RENDERERCORE_DLL ezMsgRetrieveBoneState : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRetrieveBoneState, ezMessage);

  // maps from bone name to its local transform
  ezMap<ezString, ezTransform> m_BoneTransforms;
};

/// \brief What type of physics constraint to use for a bone.
struct ezSkeletonJointType
{
  using StorageType = ezUInt8;

  enum Enum
  {
    None,  ///< The bone is not constrained, at all. It will not be connected to another bone and fall down separately.
    Fixed, ///< The bone is joined to the parent bone by a fixed joint type and can't move, at all.
    //  Hinge,
    SwingTwist, ///< The bone is joined to the parent bone and can swing and twist relative to it in limited fashion.

    Default = None,
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezSkeletonJointType);

//////////////////////////////////////////////////////////////////////////

/// \brief What to do when an animated object is not visible.
///
/// It is often important to still update animated meshes, so that animation events get handled.
/// Also even though a mesh may be invisible itself, its shadow or reflection may still be visible.
struct EZ_RENDERERCORE_DLL ezAnimationInvisibleUpdateRate
{
  using StorageType = ezUInt8;

  enum Enum
  {
    FullUpdate,
    Max60FPS,
    Max30FPS,
    Max15FPS,
    Max10FPS,
    Max5FPS,
    Pause,

    Default = Max5FPS
  };

  static ezTime GetTimeStep(ezAnimationInvisibleUpdateRate::Enum value);
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimationInvisibleUpdateRate);
