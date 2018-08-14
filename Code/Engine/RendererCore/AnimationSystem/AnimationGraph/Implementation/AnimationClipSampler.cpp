#include <PCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

ezAnimationClipSampler::ezAnimationClipSampler() = default;
ezAnimationClipSampler::~ezAnimationClipSampler() = default;


void ezAnimationClipSampler::Step(ezTime tDiff)
{
  if (m_State != ezAnimationClipSamplerState::Playing)
    return;

  SetSampleTime(m_SampleTime + tDiff * m_fPlaybackSpeed);
}

void ezAnimationClipSampler::Execute(const ezSkeleton& skeleton, ezAnimationPose& currentPose)
{
  // early out, when this is already known
  if (m_State == ezAnimationClipSamplerState::Stopped)
    return;

  // allow animation streaming, don't block
  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return;

  // make sure we now know the animation clip length
  m_ClipDuration = pAnimClip->GetDescriptor().GetDuration();

  AdjustSampleTime();

  // AdjustSampleTime() may reach the end
  if (m_State == ezAnimationClipSamplerState::Stopped)
    return;

  const auto& animDesc = pAnimClip->GetDescriptor();

  double fAnimLerp = 0;
  const ezUInt32 uiFirstFrame = animDesc.GetFrameAt(m_SampleTime, fAnimLerp);

  const auto& animatedJoints = animDesc.GetAllJointIndices();
  for (ezUInt32 b = 0; b < animatedJoints.GetCount(); ++b)
  {
    const ezHashedString sJointName = animatedJoints.GetKey(b);
    const ezUInt32 uiAnimJointIdx = animatedJoints.GetValue(b);

    ezUInt32 uiSkeletonJointIdx;
    if (skeleton.FindJointByName(sJointName, uiSkeletonJointIdx).Succeeded())
    {
      const ezTransform jointTransform = animDesc.GetJointKeyframes(uiAnimJointIdx)[uiFirstFrame];

      currentPose.SetTransform(uiSkeletonJointIdx, jointTransform.GetAsMat4());
    }
  }
}

void ezAnimationClipSampler::RestartAnimation()
{
  m_SampleTime = ezTime::Zero();
  m_State = ezAnimationClipSamplerState::Playing;
}

void ezAnimationClipSampler::SetPaused(bool pause)
{
  if (m_State == ezAnimationClipSamplerState::Stopped)
    return;

  m_State = pause ? ezAnimationClipSamplerState::Paused : ezAnimationClipSamplerState::Playing;
}

void ezAnimationClipSampler::SetAnimationClip(const ezAnimationClipResourceHandle& hAnimationClip)
{
  m_hAnimationClip = hAnimationClip;
  m_ClipDuration = ezTime::Zero();

  if (m_hAnimationClip.IsValid())
  {
    RestartAnimation();
  }
}

void ezAnimationClipSampler::SetSampleTime(ezTime time)
{
  m_SampleTime = time;
}

void ezAnimationClipSampler::SetPlaybackSpeed(float speed)
{
  m_fPlaybackSpeed = speed;
}

void ezAnimationClipSampler::SetLooping(bool loop)
{
  m_bLoop = loop;
}

void ezAnimationClipSampler::AdjustSampleTime()
{
  if (m_ClipDuration.IsZero())
  {
    m_SampleTime.SetZero();
    return;
  }

  if (m_bLoop)
  {
    while (m_SampleTime < ezTime::Zero())
    {
      // TODO: send events ?
      m_SampleTime += m_ClipDuration;
    }

    while (m_SampleTime > m_ClipDuration)
    {
      // TODO: send events ?
      m_SampleTime -= m_ClipDuration;
    }
  }
  else
  {
    if (m_SampleTime < ezTime::Zero())
    {
      // TODO: send events ?
      m_SampleTime = ezTime::Zero();
      m_State = ezAnimationClipSamplerState::Stopped;
    }
    else if (m_SampleTime > m_ClipDuration)
    {
      // TODO: send events ?
      m_SampleTime = m_ClipDuration;
      m_State = ezAnimationClipSamplerState::Stopped;
    }
  }
}
