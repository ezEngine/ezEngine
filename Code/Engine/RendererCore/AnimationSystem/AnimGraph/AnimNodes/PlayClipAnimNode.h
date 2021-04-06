#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

/// \brief Plays a single animation clip, either once or looped
class EZ_RENDERERCORE_DLL ezPlayClipAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPlayClipAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezPlayClipAnimNode

public:
  void SetAnimationClip(const char* szFile); // [ property ]
  const char* GetAnimationClip() const;      // [ property ]

  ezAnimRampUpDown m_AnimRamp;        // [ property ]
  float m_fPlaybackSpeed = 1.0f;      // [ property ]
  bool m_bApplyRootMotion = false;    // [ property ]
  bool m_bLoop = false;               // [ property ]
  bool m_bCancelWhenInactive = false; // [ property ]

private:
  void AnimationFinished(ezAnimGraph& graph);

  ezAnimationClipResourceHandle m_hAnimationClip;

  ezAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  ezAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  ezAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  ezAnimGraphTriggerOutputPin m_OnFinishedPin;  // [ property ]

  ezTime m_PlaybackTime;
  float m_fCurWeight = 0.0f;
  bool m_bIsRunning = false;
  bool m_bKeepRunning = false;

  ezAnimGraphSamplingCache* m_pSamplingCache = nullptr;
  ezAnimGraphLocalTransforms* m_pLocalTransforms = nullptr;
};
