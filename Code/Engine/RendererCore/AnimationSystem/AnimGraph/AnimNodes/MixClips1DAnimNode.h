#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezMixClips1DAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMixClips1DAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLerpClipsAnimNode

public:
  void SetAnimationClip0(const char* szFile); // [ property ]
  const char* GetAnimationClip0() const;      // [ property ]
  void SetAnimationClip1(const char* szFile); // [ property ]
  const char* GetAnimationClip1() const;      // [ property ]
  void SetAnimationClip2(const char* szFile); // [ property ]
  const char* GetAnimationClip2() const;      // [ property ]
  void SetAnimationClip3(const char* szFile); // [ property ]
  const char* GetAnimationClip3() const;      // [ property ]

  ezAnimationClipResourceHandle m_hAnimationClips[4];

private:
  ezAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  ezAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  ezAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  ezAnimGraphNumberInputPin m_LerpPin;          // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  ezAnimGraphTriggerOutputPin m_OnFinishedPin;  // [ property ]

  ezAnimState m_State; // [ property ]
};
