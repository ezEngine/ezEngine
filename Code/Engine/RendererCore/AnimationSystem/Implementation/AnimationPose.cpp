#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererFoundation/Shader/Types.h>

// clang-format off
EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPosePreparing);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPosePreparing, 1, ezRTTIDefaultAllocator<ezMsgAnimationPosePreparing>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPoseGeneration);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPoseGeneration, 1, ezRTTIDefaultAllocator<ezMsgAnimationPoseGeneration>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgAnimationPoseUpdated);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgAnimationPoseUpdated, 1, ezRTTIDefaultAllocator<ezMsgAnimationPoseUpdated>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgRopePoseUpdated);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgRopePoseUpdated, 1, ezRTTIDefaultAllocator<ezMsgRopePoseUpdated>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_IMPLEMENT_MESSAGE_TYPE(ezMsgQueryAnimationSkeleton);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMsgQueryAnimationSkeleton, 1, ezRTTIDefaultAllocator<ezMsgQueryAnimationSkeleton>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
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
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezExcludeFromScript()
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAnimationInvisibleUpdateRate, 1)
  EZ_ENUM_CONSTANT(ezAnimationInvisibleUpdateRate::FullUpdate),
  EZ_ENUM_CONSTANT(ezAnimationInvisibleUpdateRate::Max60FPS),
  EZ_ENUM_CONSTANT(ezAnimationInvisibleUpdateRate::Max30FPS),
  EZ_ENUM_CONSTANT(ezAnimationInvisibleUpdateRate::Max15FPS),
  EZ_ENUM_CONSTANT(ezAnimationInvisibleUpdateRate::Max10FPS),
  EZ_ENUM_CONSTANT(ezAnimationInvisibleUpdateRate::Max5FPS),
  EZ_ENUM_CONSTANT(ezAnimationInvisibleUpdateRate::Pause),
EZ_END_STATIC_REFLECTED_ENUM;
// clang-format on

ezTime ezAnimationInvisibleUpdateRate::GetTimeStep(ezAnimationInvisibleUpdateRate::Enum value)
{
  switch (value)
  {
    case ezAnimationInvisibleUpdateRate::FullUpdate:
      return ezTime::MakeZero();
    case ezAnimationInvisibleUpdateRate::Max60FPS:
      return ezTime::MakeFromSeconds(1.0 / 60.0);
    case ezAnimationInvisibleUpdateRate::Max30FPS:
      return ezTime::MakeFromSeconds(1.0 / 30.0);
    case ezAnimationInvisibleUpdateRate::Max15FPS:
      return ezTime::MakeFromSeconds(1.0 / 15.0);
    case ezAnimationInvisibleUpdateRate::Max10FPS:
      return ezTime::MakeFromSeconds(1.0 / 10.0);

    case ezAnimationInvisibleUpdateRate::Max5FPS:
    case ezAnimationInvisibleUpdateRate::Pause: // full pausing should be handled separately, and if something isn't fully paused, it should behave like a very low update rate
      return ezTime::MakeFromSeconds(1.0 / 5.0);

      EZ_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return ezTime::MakeZero();
}

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
