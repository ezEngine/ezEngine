#include <PCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationClipSampler.h>
#include <RendererCore/AnimationSystem/Skeleton.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <Foundation/IO/Stream.h>
#include <Core/WorldSerializer/ResourceHandleReader.h>
#include <Core/WorldSerializer/ResourceHandleWriter.h>

ezAnimationClipSampler::ezAnimationClipSampler() = default;
ezAnimationClipSampler::~ezAnimationClipSampler() = default;


void ezAnimationClipSampler::Step(ezTime tDiff)
{
  if (m_State != ezAnimationClipSamplerState::Playing)
    return;

  m_PrevSampleTime = m_SampleTime;
  m_SampleTime = m_SampleTime + tDiff * m_fPlaybackSpeed;
}

bool ezAnimationClipSampler::Execute(const ezSkeleton& skeleton, ezAnimationPose& currentPose, ezTransform* pRootMotion)
{
  // early out, when this is already known
  if (m_State == ezAnimationClipSamplerState::Stopped)
    return false;

  // allow animation streaming, don't block
  ezResourceLock<ezAnimationClipResource> pAnimClip(m_hAnimationClip);
  if (pAnimClip.GetAcquireResult() != ezResourceAcquireResult::Final)
    return false;

  // make sure we now know the animation clip length
  m_ClipDuration = pAnimClip->GetDescriptor().GetDuration();

  AdjustSampleTime();

  // AdjustSampleTime() may reach the end
  if (m_State == ezAnimationClipSamplerState::Stopped)
    return false;

  const auto& animDesc = pAnimClip->GetDescriptor();

  double fAnimLerp = 0;
  const ezUInt32 uiFirstFrame = animDesc.GetFrameAt(m_SampleTime, fAnimLerp);

  if (pRootMotion)
  {
    if (animDesc.HasRootMotion())
    {
      *pRootMotion = ComputeRootMotion(animDesc, m_PrevSampleTime, m_SampleTime);
    }
    else
    {
      pRootMotion->SetIdentity();
    }
  }

  animDesc.SetPoseToBlendedKeyframe(currentPose, skeleton, uiFirstFrame, (float)fAnimLerp);

  return true;
}


void ezAnimationClipSampler::Save(ezStreamWriter& stream) const
{
  const ezUInt8 uiVersion = 1;
  stream << uiVersion;

  stream << m_hAnimationClip;
  stream << m_bLoop;
  stream << m_fPlaybackSpeed;
}

void ezAnimationClipSampler::Load(ezStreamReader& stream)
{
  ezUInt8 uiVersion = 0;
  stream >> uiVersion;

  EZ_ASSERT_DEBUG(uiVersion == 1, "Invalid ezAnimationClipSampler version");

  stream >> m_hAnimationClip;
  stream >> m_bLoop;
  stream >> m_fPlaybackSpeed;
}

void ezAnimationClipSampler::RestartAnimation()
{
  m_SampleTime = ezTime::Zero();
  m_PrevSampleTime = ezTime::Zero();
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
}

void ezAnimationClipSampler::JumpToSampleTime(ezTime time)
{
  m_PrevSampleTime = time;
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

ezTransform ezAnimationClipSampler::ComputeRootMotion(const ezAnimationClipResourceDescriptor& animDesc, ezTime tPrev, ezTime tNow) const
{
  // TODO: handle loops when tNow wrapped around, and also when playing back an animation backwards

  const ezUInt16 uiRootMotionJoint = animDesc.GetRootMotionJoint();

  double fAnimLerpFirst = 0;
  const ezUInt32 uiFirstFrame = animDesc.GetFrameAt(tPrev, fAnimLerpFirst);

  double fAnimLerpLast = 0;
  const ezUInt32 uiLastFrame = animDesc.GetFrameAt(tNow, fAnimLerpLast);

  const ezTransform* pKeyframes = animDesc.GetJointKeyframes(uiRootMotionJoint);

  ezTransform res;
  res.SetIdentity();

  if (uiFirstFrame == uiLastFrame)
  {
    const ezTransform rm = pKeyframes[uiFirstFrame];

    const float fFraction = (float)(fAnimLerpLast - fAnimLerpFirst);

    res.m_vPosition = fFraction * rm.m_vPosition;
    res.m_qRotation.SetSlerp(ezQuat::IdentityQuaternion(), rm.m_qRotation, fFraction);
  }
  else
  {
    {
      const ezTransform rm = pKeyframes[uiFirstFrame];

      const float fFraction = (float)(1.0 - fAnimLerpFirst);

      res.m_vPosition += fFraction * rm.m_vPosition;
      // res.m_qRotation.SetSlerp(ezQuat::IdentityQuaternion(), rm.m_qRotation, fFraction);
    }

    for (ezUInt32 i = uiFirstFrame + 1; i < uiLastFrame; ++i)
    {
      const ezTransform rm = pKeyframes[i];

      res.m_vPosition += rm.m_vPosition;
      // rotation
    }


    {
      const ezTransform rm = pKeyframes[uiLastFrame];

      const float fFraction = (float)fAnimLerpLast;

      res.m_vPosition += fFraction * rm.m_vPosition;
      // res.m_qRotation.SetSlerp(ezQuat::IdentityQuaternion(), rm.m_qRotation, fFraction);
    }
  }

  return res;
}
