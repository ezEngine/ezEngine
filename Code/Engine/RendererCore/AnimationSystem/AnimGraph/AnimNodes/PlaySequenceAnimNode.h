#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezPlaySequenceAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlaySequenceAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPlayClipAnimNode

public:
  void SetStartClip(const char* szFile); // [ property ]
  const char* GetStartClip() const;      // [ property ]

  void SetMiddleClip(const char* szFile); // [ property ]
  const char* GetMiddleClip() const;      // [ property ]

  void SetEndClip(const char* szFile); // [ property ]
  const char* GetEndClip() const;      // [ property ]

  ezAnimRampUpDown m_AnimRamp;           // [ property ]
  float m_fPlaybackSpeed = 1.0f;         // [ property ]
  bool m_bApplyRootMotion = false;       // [ property ]
  bool m_bAllowLoopInterruption = false; // [ property ]
  ezTime m_StartToMiddleCrossFade;       // [ property ]
  ezTime m_LoopCrossFade;                // [ property ]
  ezTime m_MiddleToEndCrossFade;         // [ property ]

  ezAnimationClipResourceHandle m_hStartClip;
  ezAnimationClipResourceHandle m_hMiddleClip;
  ezAnimationClipResourceHandle m_hEndClip;

private:
  ezAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  ezAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  ezAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  ezAnimGraphTriggerOutputPin m_OnFinishedPin;  // [ property ]

  enum class State : ezUInt8
  {
    Off,
    Start,
    Middle,
    End,
  };

  State m_State = State::Off;

  ezAnimationClipResourceHandle m_hPlayingClips[2];

  ezTime m_PlaybackTime;
  bool m_bStopWhenPossible = false;
  float m_fCurWeight = 0.0f;
};
