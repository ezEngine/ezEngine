#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPosePreparing);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPosePreparing, 1, ezRTTIDefaultAllocator<ezMsgAnimationPosePreparing>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPoseUpdated);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPoseUpdated, 1, ezRTTIDefaultAllocator<ezMsgAnimationPoseUpdated>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgQueryAnimationSkeleton);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgQueryAnimationSkeleton, 1, ezRTTIDefaultAllocator<ezMsgQueryAnimationSkeleton>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgApplyRootMotion);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgApplyRootMotion, 1, ezRTTIDefaultAllocator<ezMsgApplyRootMotion>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Translation", m_vTranslation),
    EZ_MEMBER_PROPERTY("RotationX", m_RotationX),
    EZ_MEMBER_PROPERTY("RotationY", m_RotationY),
    EZ_MEMBER_PROPERTY("RotationZ", m_RotationZ),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezSkinningSpaceAnimationPose::ezSkinningSpaceAnimationPose() = default;
ezSkinningSpaceAnimationPose::~ezSkinningSpaceAnimationPose() = default;

void ezSkinningSpaceAnimationPose::Clear()
{
  m_Transforms.Clear();
  m_Transforms.Compact();
}

void ezSkinningSpaceAnimationPose::Configure(ezUInt32 uiNumTransforms)
{
  m_Transforms.SetCountUninitialized(uiNumTransforms);
}

void ezSkinningSpaceAnimationPose::MapModelSpacePoseToSkinningSpace(const ezHashTable<ezHashedString, ezMeshResourceDescriptor::BoneData>& bones, const ezSkeleton& skeleton, ezArrayPtr<const ezMat4> modelSpaceTransforms)
{
  Configure(bones.GetCount());

  for (auto itBone : bones)
  {
    const ezUInt16 uiJointIdx = skeleton.FindJointByName(itBone.Key());

    if (uiJointIdx == ezInvalidJointIndex)
      continue;

    m_Transforms[itBone.Value().m_uiBoneIndex] = modelSpaceTransforms[uiJointIdx] * itBone.Value().m_GlobalInverseBindPoseMatrix;
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationPose);
