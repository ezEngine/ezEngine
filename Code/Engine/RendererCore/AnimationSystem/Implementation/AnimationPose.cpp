#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/Shader/Types.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPosePreparing);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPosePreparing, 1, ezRTTIDefaultAllocator<ezMsgAnimationPosePreparing>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPoseUpdated);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPoseUpdated, 1, ezRTTIDefaultAllocator<ezMsgAnimationPoseUpdated>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPoseProposal);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPoseProposal, 1, ezRTTIDefaultAllocator<ezMsgAnimationPoseProposal>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgRopePoseUpdated);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgRopePoseUpdated, 1, ezRTTIDefaultAllocator<ezMsgRopePoseUpdated>)
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

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgRetrieveBoneState);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgRetrieveBoneState, 1, ezRTTIDefaultAllocator<ezMsgRetrieveBoneState>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezMsgAnimationPoseUpdated::ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& ref_mFullTransform) const
{
  ref_mFullTransform = m_pRootTransform->GetAsMat4() * m_ModelTransforms[uiJointIndex];
}

void ezMsgAnimationPoseUpdated::ComputeFullBoneTransform(const ezMat4& mRootTransform, const ezMat4& mModelTransform, ezMat4& ref_mFullTransform, ezQuat& ref_qRotationOnly)
{
  ref_mFullTransform = mRootTransform * mModelTransform;

  // the bone might contain (non-uniform) scaling and mirroring, which the quaternion can't represent
  // so reconstruct a representable rotation matrix
  ref_qRotationOnly.ReconstructFromMat4(ref_mFullTransform);
}

void ezMsgAnimationPoseUpdated::ComputeFullBoneTransform(ezUInt32 uiJointIndex, ezMat4& ref_mFullTransform, ezQuat& ref_qRotationOnly) const
{
  ComputeFullBoneTransform(m_pRootTransform->GetAsMat4(), m_ModelTransforms[uiJointIndex], ref_mFullTransform, ref_qRotationOnly);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_Implementation_AnimationPose);
