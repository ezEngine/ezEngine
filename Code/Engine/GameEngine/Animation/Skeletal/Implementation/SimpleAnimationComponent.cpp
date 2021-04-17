#include <GameEnginePCH.h>

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
EZ_BEGIN_COMPONENT_TYPE(ezSimpleAnimationComponent, 1, ezComponentMode::Static);
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("AnimationClip", GetAnimationClipFile, SetAnimationClipFile)->AddAttributes(new ezAssetBrowserAttribute("Animation Clip")),
    EZ_ENUM_MEMBER_PROPERTY("AnimationMode", ezPropertyAnimMode, m_AnimationMode),
    EZ_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ENUM_MEMBER_PROPERTY("RootMotionMode", ezRootMotionMode, m_RootMotionMode),
  }
  EZ_END_PROPERTIES;

  EZ_BEGIN_MESSAGESENDERS
  {
    EZ_MESSAGE_SENDER(m_EventTrackMsgSender),
    EZ_MESSAGE_SENDER(m_ReachedEndMsgSender),
  }
  EZ_END_MESSAGESENDERS;

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

void ezSimpleAnimationComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_AnimationMode;
  s << m_fSpeed;
  s << m_hAnimationClip;
  s << m_RootMotionMode;
}

void ezSimpleAnimationComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_AnimationMode;
  s >> m_fSpeed;
  s >> m_hAnimationClip;
  s >> m_RootMotionMode;
}

void ezSimpleAnimationComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  ezMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  m_hSkeleton = msg.m_hSkeleton;
}

void ezSimpleAnimationComponent::SetAnimationClip(const ezAnimationClipResourceHandle& hResource)
{
  m_hAnimationClip = hResource;
}

const ezAnimationClipResourceHandle& ezSimpleAnimationComponent::GetAnimationClip() const
{
  return m_hAnimationClip;
}

void ezSimpleAnimationComponent::SetAnimationClipFile(const char* szFile)
{
  ezAnimationClipResourceHandle hResource;

  if (!ezStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = ezResourceManager::LoadResource<ezAnimationClipResource>(szFile);
  }

  SetAnimationClip(hResource);
}

const char* ezSimpleAnimationComponent::GetAnimationClipFile() const
{
  if (!m_hAnimationClip.IsValid())
    return "";

  return m_hAnimationClip.GetResourceID();
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

  ezResourceLock<ezAnimationClipResource> pAnimation(m_hAnimationClip, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pAnimation.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  const ezAnimationClipResourceDescriptor& animDesc = pAnimation->GetDescriptor();

  m_Duration = animDesc.GetDuration();

  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  if (!UpdatePlaybackTime(tDiff, animDesc.m_EventTrack))
    return;

  ezResourceLock<ezSkeletonResource> pSkeleton(m_hSkeleton, ezResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pSkeleton.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  ezAnimPoseGenerator poseGen;
  poseGen.Reset(pSkeleton.GetPointer());

  auto& cmdSample = poseGen.AllocCommandSampleTrack(0);
  cmdSample.m_hAnimationClip = m_hAnimationClip;
  cmdSample.m_fNormalizedSamplePos = m_fNormalizedPlaybackPosition;

  auto& cmdL2M = poseGen.AllocCommandLocalToModelPose();
  cmdL2M.m_Inputs.PushBack(cmdSample.GetCommandID());
  cmdL2M.m_pSendLocalPoseMsgTo = GetOwner();

  auto& cmdOut = poseGen.AllocCommandModelPoseToOutput();
  cmdOut.m_Inputs.PushBack(cmdL2M.GetCommandID());

  auto pose = poseGen.GeneratePose();

  if (pose.IsEmpty())
    return;

  // inform child nodes/components that a new pose is available
  {
    ezMsgAnimationPoseUpdated msg;
    msg.m_pRootTransform = &pSkeleton->GetDescriptor().m_RootTransform;
    msg.m_pSkeleton = &pSkeleton->GetDescriptor().m_Skeleton;
    msg.m_ModelTransforms = pose;

    GetOwner()->SendMessageRecursive(msg);
  }

  if (m_RootMotionMode != ezRootMotionMode::Ignore)
  {
    ezVec3 vRootMotion = tDiff.AsFloatInSeconds() * m_fSpeed * animDesc.m_vConstantRootMotion;

    const bool bReverse = GetUserFlag(0);
    if (bReverse)
    {
      vRootMotion = -vRootMotion;
    }

    ezRootMotionMode::Apply(m_RootMotionMode, GetOwner(), vRootMotion, ezQuat::IdentityQuaternion());
  }
}

bool ezSimpleAnimationComponent::UpdatePlaybackTime(ezTime tDiff, const ezEventTrack& eventTrack)
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

  const float tDiffNorm = static_cast<float>(tDiff.GetSeconds() / m_Duration.GetSeconds());
  const float tPrefNorm = m_fNormalizedPlaybackPosition;

  ezHybridArray<ezHashedString, 8> events;

  switch (m_AnimationMode)
  {
    case ezPropertyAnimMode::Once:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;
      m_fNormalizedPlaybackPosition = ezMath::Clamp(m_fNormalizedPlaybackPosition, 0.0f, 1.0f);

      eventTrack.Sample(tPrefNorm * m_Duration, m_fNormalizedPlaybackPosition * m_Duration, events);

      break;
    }

    case ezPropertyAnimMode::Loop:
    {
      m_fNormalizedPlaybackPosition += tDiffNorm * m_fSpeed;

      if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        eventTrack.Sample(tPrefNorm * m_Duration, ezTime::Zero(), events);

        m_fNormalizedPlaybackPosition += 1.0f;

        eventTrack.Sample(m_Duration, m_fNormalizedPlaybackPosition * m_Duration, events);
      }
      else if (m_fNormalizedPlaybackPosition > 1.0f)
      {
        eventTrack.Sample(tPrefNorm * m_Duration, m_Duration, events);

        m_fNormalizedPlaybackPosition -= 1.0f;

        eventTrack.Sample(ezTime::Zero(), m_fNormalizedPlaybackPosition * m_Duration, events);
      }
      else
      {
        eventTrack.Sample(tPrefNorm * m_Duration, m_fNormalizedPlaybackPosition * m_Duration, events);
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

        eventTrack.Sample(tPrefNorm * m_Duration, m_Duration, events);

        m_fNormalizedPlaybackPosition = 2.0f - m_fNormalizedPlaybackPosition;

        eventTrack.Sample(m_Duration, m_fNormalizedPlaybackPosition * m_Duration, events);
      }
      else if (m_fNormalizedPlaybackPosition < 0.0f)
      {
        SetUserFlag(0, !bReverse);

        eventTrack.Sample(tPrefNorm * m_Duration, ezTime::Zero(), events);

        m_fNormalizedPlaybackPosition = -m_fNormalizedPlaybackPosition;

        eventTrack.Sample(ezTime::Zero(), m_fNormalizedPlaybackPosition * m_Duration, events);
      }
      else
      {
        eventTrack.Sample(tPrefNorm * m_Duration, m_fNormalizedPlaybackPosition * m_Duration, events);
      }

      break;
    }
  }

  for (const ezHashedString& sEvent : events)
  {
    ezMsgGenericEvent msg;
    msg.m_sMessage = sEvent;
    m_EventTrackMsgSender.SendEventMessage(msg, this, GetOwner());
  }

  return tPrefNorm != m_fNormalizedPlaybackPosition;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_SimpleAnimationComponent);
