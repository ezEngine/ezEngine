#pragma once

#include <Foundation/Time/Time.h>
#include <RendererCore/AnimationSystem/AnimationGraph/AnimationGraphNode.h>

enum class ezAnimationClipSamplerState
{
  Stopped,
  Paused,
  Playing,
};

class EZ_RENDERERCORE_DLL ezAnimationClipSampler : public ezAnimationGraphNode
{
public:
  ezAnimationClipSampler();
  ~ezAnimationClipSampler();

  virtual void Step(ezTime tDiff) override;
  virtual void Execute(const ezSkeleton& skeleton, ezAnimationPose& currentPose) override;

  void RestartAnimation();
  void SetPaused(bool pause);

  void SetAnimationClip(const ezAnimationClipResourceHandle& hAnimationClip);
  const ezAnimationClipResourceHandle& GetAnimationClip() const { return m_hAnimationClip; }
  
  void SetSampleTime(ezTime time);

  ezTime GetClipDuration() const { return m_ClipDuration; }
  // ezTime GetDurationAtCurrentSpeed() const
  // ezTime GetRemainingDurationAtCurrentSpeed() const

  void SetPlaybackSpeed(float speed);
  float GetPlaybackSpeed() const { return m_fPlaybackSpeed; }

  void SetLooping(bool loop);
  bool GetLooping() const { return m_bLoop; }

private:
  void AdjustSampleTime();

  ezAnimationClipSamplerState m_State = ezAnimationClipSamplerState::Stopped;
  ezAnimationClipResourceHandle m_hAnimationClip;
  ezTime m_SampleTime;
  ezTime m_ClipDuration;
  float m_fPlaybackSpeed = 1.0f;
  bool m_bLoop = false;
};
