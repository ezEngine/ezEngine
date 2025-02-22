#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/Skeletal/SimpleAnimationComponent.h>
#include <RendererCore/AnimationSystem/AnimPoseGenerator.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/skeleton.h>

using namespace ozz;
using namespace ozz::animation;
using namespace ozz::math;

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezSimpleAnimationComponent, 3, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_RESOURCE_MEMBER_PROPERTY("AnimationClip", m_hAnimationClip)->AddAttributes(new ezAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
    EZ_ENUM_MEMBER_PROPERTY("AnimationMode", ezPropertyAnimMode, m_AnimationMode),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ENUM_MEMBER_PROPERTY("RootMotionMode", ezRootMotionMode, m_RootMotionMode),
    EZ_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", ezAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
    EZ_MEMBER_PROPERTY("EnableIK", m_bEnableIK),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_ATTRIBUTES
  {
      new ezCategoryAttribute("Animation"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE
// clang-format on

ezSimpleAnimationComponent::ezSimpleAnimationComponent() = default;
ezSimpleAnimationComponent::~ezSimpleAnimationComponent() = default;

void ezSimpleAnimationComponent::SerializeComponent(ezWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_AnimationMode;
  s << m_fSpeed;
  s << m_hAnimationClip;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
  s << m_bEnableIK;
}

void ezSimpleAnimationComponent::DeserializeComponent(ezWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const ezUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_AnimationMode;
  s >> m_fSpeed;
  s >> m_hAnimationClip;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }

  if (uiVersion >= 3)
  {
    s >> m_bEnableIK;
  }
}

void ezSimpleAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  m_hSkeleton = msg.m_hSkeleton;
}

void ezSimpleAnimationComponent::SetNormalizedPlaybackPosition(float fPosition)
{
  m_fNormalizedPlaybackPosition = fPosition;

  // force update next time
  SetUserFlag(1, true);
}

void ezSimpleAnimationComponent::Update()
{
  if (!m_hSkeleton.IsValid() || !m_hAnimationClip.IsValid())
    return;

  if (m_fSpeed == 0.0f && !GetUserFlag(1))
    return;

  ezTime tMinStep = ezTime::MakeFromSeconds(0);
  ezVisibilityState::Enum visType = GetOwner()->GetVisibilityState();

  if (visType != ezVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == ezAnimationInvisibleUpdateRate::Pause && visType == ezVisibilityState::Invisible)
      return;

    tMinStep = ezAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  const bool bVisible = visType != ezVisibilityState::Invisible;

  ezResourceLock<ezAnimationClipResource> pAnimation(m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ezTime tDiff = m_ElapsedTimeSinceUpdate;
  m_ElapsedTimeSinceUpdate = ezTime::MakeZero();

  const ezAnimationClipResourceDescriptor& animDesc = pAnimation->GetDescriptor();

  m_Duration = animDesc.GetDuration();

  const float fPrevPlaybackPos = m_fNormalizedPlaybackPosition;

  ezAnimPoseEventTrackSampleMode mode = ezAnimPoseEventTrackSampleMode::None;

  if (!UpdatePlaybackTime(tDiff, animDesc.m_EventTrack, mode))
    return;

  if (animDesc.m_EventTrack.IsEmpty())
  {
    mode = ezAnimPoseEventTrackSampleMode::None;
  }

  // no need to do anything, if we can't get events and are currently invisible
  if (!bVisible && mode == ezAnimPoseEventTrackSampleMode::None && m_RootMotionMode == ezRootMotionMode::Ignore)
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezAnimPoseGenerator poseGen;
  poseGen.Reset(pSkeleton.GetPointer(), GetOwner());

  auto& cmdSample = poseGen.AllocCommandSampleTrack(0);
  cmdSample.m_hAnimationClip = m_hAnimationClip;
  cmdSample.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;
  cmdSample.m_fPreviousNormalizedSamplePos = fPrevPlaybackPos;
  cmdSample.m_EventSampling = mode;

  if (bVisible)
  {
    auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
    cmdL2M.m_pSendLocalPoseMsgTo = GetOwner();

    if (animDesc.m_bAdditive)
    {
      auto& cmdComb = poseGen.AllocCommandCombinePoses();
      cmdComb.m_Inputs.PushBack(cmdSample.GetCommandID());
      cmdComb.m_InputWeights.PushBack(1.0f);

      cmdL2M.m_Inputs.PushBack(cmdComb.GetCommandID());
    }
    else
    {
      cmdL2M.m_Inputs.PushBack(cmdSample.GetCommandID());
    }

    ezAnimPoseGeneratorCommandID prevCmdID = cmdL2M.GetCommandID();
    poseGen.SetFinalCommand(prevCmdID);
  }

  poseGen.UpdatePose(m_bEnableIK);

  if (m_RootMotionMode != ezRootMotionMode::Ignore)
  {
    ezVec3 vRootMotion = tDiff.AsFloatInSeconds() * m_fSpeed * animDesc.m_vConstantRootMotion;

    const bool bReverse = GetUserFlag(0);
    if (bReverse)
    {
      vRootMotion = -vRootMotion;
    }

    // only applies positional root motion
    ezRootMotionMode::Apply(m_RootMotionMode, GetOwner(), vRootMotion, ezAngle(), ezAngle(), ezAngle());
  }

  if (poseGen.GetCurrentPose().IsEmpty())
    return;

  // inform child nodes/components that a new pose is available
  {
    ezMsgAnimationPoseUpdated msg2;
    msg2.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg2.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg2.m_ModelTransforms = poseGen.GetCurrentPose();

    // recursive, so that objects below the mesh can also listen in on these changes
    // for example bone attachments
    GetOwner()->SendMessageRecursive(msg2);

    if (msg2.m_bContinueAnimating == false)
    {
      SetActiveFlag(false);
    }
  }
}

bool ezSimpleAnimationComponent::UpdatePlaybackTime(ezTime tDiff, const ezEventTrack& eventTrack, ezAnimPoseEventTrackSampleMode& out_trackSampling)
{
  if (tDiff.IsZero() || m_fSpeed == 0.0f)
  {
    if (GetUserFlag(1))
    {
      SetUserFlag(1, false);
      return true;
    }

    return false;
  }

  out_trackSampling = ezAnimPoseEventTrackSampleMode::OnlyBetween;

  const float tDiffNorm = static_cast<float>(tDiff.GetSeconds() / m_Duration.GetSeconds());
  const float tPrefNorm = m_fNormalizedPlaybackPosition;

  switch (m_AnimationMode)
  {
    case ezPropertyAnimMode::Once:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;
      m_fNormalizedPlaybackPosition = ezMath::Clamp(m_fNormalizedPlaybackPosition, 0.0f, 1.0f);
      break;
    }

    case ezPropertyAnimMode::Loop:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        m_fNormalizedPlaybackPosition += 1.0f;

        out_trackSampling = ezAnimPoseEventTrackSampleMode::LoopAtStart;
      }
      else if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        m_fNormalizedPlaybackPosition -= 1.0f;

        out_trackSampling = ezAnimPoseEventTrackSampleMode::LoopAtEnd;
      }

      break;
    }

    case ezPropertyAnimMode::BackAndForth:
    {
      const bool bReverse = GetUserFlag(0);

      if (bReverse)
        m_fNormalizedPlaybackPosition -= tDiffNorm * m_fSpeed;
      else
        m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = 2.0f - m_fNormalizedPlaybackPosition;

        out_trackSampling = ezAnimPoseEventTrackSampleMode::BounceAtEnd;
      }
      else if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        SetUserFlag(0, !bReverse);

        m_fNormalizedPlaybackPosition = -m_fNormalizedPlaybackPosition;

        out_trackSampling = ezAnimPoseEventTrackSampleMode::BounceAtStart;
      }

      break;
    }
  }

  return tPrefNorm != m_fNormalizedPlaybackPosition;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_SimpleAnimationComponent);
