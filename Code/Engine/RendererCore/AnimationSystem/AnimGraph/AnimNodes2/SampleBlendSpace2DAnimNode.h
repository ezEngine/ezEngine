#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

struct EZ_RENDERERCORE_DLL ezAnimationClip2D
{
  ezAnimationClipResourceHandle m_hAnimation;
  ezVec2 m_vPosition;

  void SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimationClip2D);

class EZ_RENDERERCORE_DLL ezSampleBlendSpace2DAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSampleBlendSpace2DAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSampleBlendSpace1DAnimNode

public:
  ezSampleBlendSpace2DAnimNode();
  ~ezSampleBlendSpace2DAnimNode();

  void SetCenterClipFile(const char* szFile);
  const char* GetCenterClipFile() const;

private:
  ezAnimationClipResourceHandle m_hCenterClip;        // [ property ]
  ezHybridArray<ezAnimationClip2D, 8> m_Clips;        // [ property ]
  ezTime m_InputResponse = ezTime::Milliseconds(100); // [ property ]
  bool m_bLoop = true;                                // [ property ]
  bool m_bApplyRootMotion = false;                    // [ property ]
  float m_fPlaybackSpeed = 1.0f;                      // [ property ]

  ezAnimGraphTriggerInputPin m_InStart;        // [ property ]
  ezAnimGraphBoolInputPin m_InLoop;            // [ property ]
  ezAnimGraphNumberInputPin m_InSpeed;         // [ property ]
  ezAnimGraphNumberInputPin m_InCoordX;        // [ property ]
  ezAnimGraphNumberInputPin m_InCoordY;        // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]

  struct ClipToPlay
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiIndex;
    float m_fWeight = 1.0f;
  };

  struct InstanceState
  {
    bool m_bPlaying = false;
    ezTime m_CenterPlaybackTime;
    float m_fOtherPlaybackPosNorm = 0.0f;
    float m_fLastValueX = 0.0f;
    float m_fLastValueY = 0.0f;
  };

  void UpdateCenterClipPlaybackTime(InstanceState* pState, ezAnimGraph& graph, ezTime tDiff, ezAnimPoseEventTrackSampleMode& out_eventSamplingCenter);
  void PlayClips(InstanceState* pState, ezAnimGraph& graph, ezTime tDiff, ezArrayPtr<ClipToPlay> clips, ezUInt32 uiMaxWeightClip);
  void ComputeClipsAndWeights(const ezVec2& p, ezDynamicArray<ClipToPlay>& out_Clips, ezUInt32& out_uiMaxWeightClip);
};