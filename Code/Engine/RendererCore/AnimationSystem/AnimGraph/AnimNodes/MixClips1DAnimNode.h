#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

struct EZ_RENDERERCORE_DLL ezAnimClip1D
{
  ezAnimationClipResourceHandle m_hAnimation;
  float m_fPosition;

  void SetAnimationFile(const char* sz);
  const char* GetAnimationFile() const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimClip1D);

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
  // ezMixClips1DAnimNode

public:
  ezHybridArray<ezAnimClip1D, 3> m_Clips; // [ property ]

private:
  ezAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  ezAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  ezAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  ezAnimGraphNumberInputPin m_LerpPin;          // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  ezAnimGraphTriggerOutputPin m_OnFadeOutPin;  // [ property ]

  ezAnimState m_State; // [ property ]
};
