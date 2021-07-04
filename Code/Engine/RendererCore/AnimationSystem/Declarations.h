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

using ezSkeletonResourceHandle = ezTypedResourceHandle<class ezSkeletonResource>;

#define ezInvalidJointIndex 0xFFFFu

namespace ozz::animation
{
  class Skeleton;
}

struct ezSkeletonJointGeometryType
{
  typedef ezUInt8 StorageType;

  enum Enum
  {
    None = 0,
    Capsule,
    Sphere,
    Box,

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

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct EZ_RENDERERCORE_DLL ezMsgAnimationPoseUpdated : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationPoseUpdated, ezMessage);

  void ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& fullTransform) const;
  void ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& fullTransform, ezQuat& rotationOnly) const;

  const ezTransform* m_pRootTransform = nullptr;
  const ezSkeleton* m_pSkeleton = nullptr;
  ezArrayPtr<const ezMat4> m_ModelTransforms;
  bool m_bContinueAnimating = true;
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
