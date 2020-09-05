#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Communication/Message.h>
#include <RendererCore/RendererCoreDLL.h>

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

/// \brief Used by components that skin a mesh to inform children whenever a new pose has been computed.
///
/// This can be used by child nodes/components to synchronize their state to the new animation pose.
/// The message is sent while the pose is in object space.
/// Both skeleton and pose pointer are always valid.
struct EZ_RENDERERCORE_DLL ezMsgAnimationPoseUpdated : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgAnimationPoseUpdated, ezMessage);

  const ezSkeleton* m_pSkeleton = nullptr;
  // const ezAnimationPose* m_pPose = nullptr;

  ezArrayPtr<const ezMat4> m_ModelTransforms;
};

struct EZ_RENDERERCORE_DLL ezMsgQueryAnimationSkeleton : public ezMessage
{
  EZ_DECLARE_MESSAGE_TYPE(ezMsgQueryAnimationSkeleton, ezMessage);

  ezSkeletonResourceHandle m_hSkeleton;
};
