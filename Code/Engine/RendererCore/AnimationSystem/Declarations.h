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

/// What shape is used to approximate a bone's geometry
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

/// Sent before converting local-space poses to model-space, allowing last-minute modifications.
///
/// This message is sent during pose generation after all blending is complete but before the
/// local to model space conversion. The transforms are still in local space (relative to parent)
/// and in ozz's Structure-of-Arrays (SoA) format.
///
/// **Use cases:**
/// - Modify specific bones before final pose computation
/// - Apply procedural adjustments that need to propagate to children
/// - Override animation data for specific joints
///
/// **Timing:** Sent from ezAnimPoseGenerator during UpdatePose(), before LocalToModelPose command execution
///
/// **Note:** Modifications at this stage will affect all child bones in the hierarchy.
struct EZ_RENDERERCORE_DLL ezMsgAnimationPosePreparing : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationPosePreparing, ezMessage);

  const ezSkeleton* m_pSkeleton = nullptr;
  ezArrayPtr<ozz::math::SoaTransform> m_LocalTransforms;
};

/// Sent during pose generation to allow components to inject additional commands (IK, constraints, etc.).
///
/// This message provides access to the ezAnimPoseGenerator during pose construction, allowing components
/// to add their own commands to the command DAG. This is the primary extension point for inverse kinematics,
/// powered ragdolls, and other pose modifications that need access to the current pose.
///
/// **Typical workflow:**
/// 1. Base pose is generated and converted to model space
/// 2. This message is sent to child components with the pose generator
/// 3. Components query the current pose via m_pGenerator->GetCurrentPose()
/// 4. Components add IK or other commands (AimIK, TwoBoneIK, custom constraints)
/// 5. New commands reference the existing final command as input
/// 6. Component updates the final command via SetFinalCommand()
/// 7. Pose generator re-evaluates with the additional commands
///
/// **Timing:** Sent from ezAnimController::Update() after initial pose generation but before final output
/// ```
struct EZ_RENDERERCORE_DLL ezMsgInjectPoseCommands : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgInjectPoseCommands, ezMessage);

  ezAnimPoseGenerator* m_pGenerator = nullptr;

  /// Current execution pass number. Components compare this against their m_uiOrder property to determine if they should execute in this pass.
  ezUInt16 m_uiOrderNow = 0;

  /// Lowest order number that any component wants to be executed at. Components set this to schedule a future pass. 0xFFFF means no further passes needed.
  ezUInt16 m_uiOrderNext = 0xFFFF;
};

/// Sent after a new animation pose has been fully computed, providing the final bone transforms.
///
/// This message is sent to child components after all pose generation, blending, and IK have been applied.
/// The transforms are in model space (relative to skeleton root) and ready for rendering or further processing.
///
/// **Use cases:**
/// - Apply pose to ezSkinnedMeshComponent for rendering
/// - Attach objects to specific bones (weapons, accessories)
/// - Sync particle effects to animation state
/// - Update physics bodies for animated bones
///
/// **Timing:** Sent from the component that owns the animation controller after UpdatePose() completes
struct EZ_RENDERERCORE_DLL ezMsgAnimationPoseUpdated : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationPoseUpdated, ezMessage);

  /// Calculates world space transform.
  ///
  /// mRootTransform may contain (non-uniform) scaling and mirroring, which the quaternion can't represent.
  /// Therefore ref_mFullTransform is a full 4x4 matrix and ref_qRotationOnly gets reconstructed from it in a more elaborate way.
  static void ComputeFullBoneTransform(const ezMat4& mRootTransform, const ezMat4& mModelTransform, ezMat4& ref_mFullTransform, ezQuat& ref_qRotationOnly);
  void ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& ref_mFullTransform) const;
  void ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& ref_mFullTransform, ezQuat& ref_qRotationOnly) const;

  /// World transform of the skeleton root
  const ezTransform* m_pRootTransform = nullptr;
  const ezSkeleton* m_pSkeleton = nullptr;

  /// Bone transforms relative to skeleton root
  ezArrayPtr<const ezMat4> m_ModelTransforms;
  bool m_bContinueAnimating = true;
};

/// Used by components that do rope simulation and rendering.
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

/// The animated mesh component listens to this message and 'answers' by filling out the skeleton resource handle.
///
/// This can be used by components that require a skeleton, to ask the nearby components to provide it to them.
struct EZ_RENDERERCORE_DLL ezMsgQueryAnimationSkeleton : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgQueryAnimationSkeleton, ezMessage);

  ezSkeletonResourceHandle m_hSkeleton;
};

/// Sent when root motion has been extracted from animations, providing movement data for the character.
///
/// Root motion is the translation and rotation of the skeleton root extracted from animation clips.
/// It allows animations to drive character movement in the world, preventing "sliding feet" and
/// enabling animation-driven locomotion.
///
/// **Note:** Root motion must be enabled on animation sampling nodes via m_fRootMotionAmount (0-1 blend factor).
struct EZ_RENDERERCORE_DLL ezMsgApplyRootMotion : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgApplyRootMotion, ezMessage);

  ezVec3 m_vTranslation;
  ezAngle m_RotationX;
  ezAngle m_RotationY;
  ezAngle m_RotationZ;
};

/// Queries the local transforms of each bone in an object with a skeleton
///
/// Used to retrieve the pose of a ragdoll after simulation.
struct EZ_RENDERERCORE_DLL ezMsgRetrieveBoneState : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgRetrieveBoneState, ezMessage);

  // maps from bone name to its local transform
  ezMap<ezString, ezTransform> m_BoneTransforms;
};

/// What type of physics constraint to use for a bone.
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

/// What to do when an animated object is not visible.
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
