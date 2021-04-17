#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

struct EZ_RENDERERCORE_DLL ezAnimClip2D
{
  ezAnimationClipResourceHandle m_hAnimation;
  ezVec2 m_vPosition;

  void SetAnimationFile(const char* sz);
  const char* GetAnimationFile() const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimClip2D);

class EZ_RENDERERCORE_DLL ezMixClips2DAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMixClips2DAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezMixClips2DAnimNode

public:
  ezAnimRampUpDown m_AnimRamp;     // [ property ]
  float m_fPlaybackSpeed = 1.0f;   // [ property ]
  bool m_bApplyRootMotion = false; // [ property ]
  bool m_bLoop = true;             // [ property ]

  void SetCenterClipFile(const char* sz);
  const char* GetCenterClipFile() const;

  ezAnimationClipResourceHandle m_hCenterClip; // [ property ]
  ezHybridArray<ezAnimClip2D, 8> m_Clips;      // [ property ]

private:
  ezAnimGraphTriggerInputPin m_ActivePin;       // [ property ]
  ezAnimGraphBoneWeightsInputPin m_WeightsPin;  // [ property ]
  ezAnimGraphNumberInputPin m_SpeedPin;         // [ property ]
  ezAnimGraphNumberInputPin m_XCoordPin;        // [ property ]
  ezAnimGraphNumberInputPin m_YCoordPin;        // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin; // [ property ]
  ezAnimGraphTriggerOutputPin m_OnFinishedPin;  // [ property ]

  struct ClipToPlay
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiIndex;
    float m_fWeight = 1.0f;
  };

  void UpdateCenterClipPlaybackTime(ezAnimGraph& graph, ezTime tDiff);
  void PlayClips(ezAnimGraph& graph, ezTime tDiff, ezArrayPtr<ClipToPlay> clips);
  void ComputeClipsAndWeights(const ezVec2& p, ezDynamicArray<ClipToPlay>& out_Clips);

  ezTime m_CenterPlaybackTime;
  ezAnimState m_State;
};
