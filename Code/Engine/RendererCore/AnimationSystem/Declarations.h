#pragma once

#include <RendererCore/RendererCoreDLL.h>
#include <Foundation/Communication/Message.h>

class ezSkeleton;
class ezAnimationPose;
struct ezSkeletonResourceDescriptor;
class ezEditableSkeletonJoint;
struct ezAnimationClipResourceDescriptor;

#define ezInvalidJointIndex 0xFFFFu

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
  const ezAnimationPose* m_pPose = nullptr;
};

