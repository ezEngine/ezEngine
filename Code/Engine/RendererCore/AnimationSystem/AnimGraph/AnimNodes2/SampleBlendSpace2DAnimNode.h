#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimController.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

struct EZ_RENDERERCORE_DLL ezAnimationClip2D
{
  ezHashedString m_sClip;
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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSampleBlendSpace1DAnimNode

public:
  ezSampleBlendSpace2DAnimNode();
  ~ezSampleBlendSpace2DAnimNode();

  void SetCenterClipFile(const char* szFile);
  const char* GetCenterClipFile() const;

private:
  ezHashedString m_sCenterClip;                               // [ property ]
  ezHybridArray<ezAnimationClip2D, 8> m_Clips;                // [ property ]
  ezTime m_InputResponse = ezTime::MakeFromMilliseconds(100); // [ property ]
  bool m_bLoop = true;                                        // [ property ]
  float m_fRootMotionAmount = 0.0f;                           // [ property ]
  float m_fPlaybackSpeed = 1.0f;                              // [ property ]

  ezAnimGraphTriggerInputPin m_InStart;                       // [ property ]
  ezAnimGraphBoolInputPin m_InLoop;                           // [ property ]
  ezAnimGraphNumberInputPin m_InSpeed;                        // [ property ]
  ezAnimGraphNumberInputPin m_InCoordX;                       // [ property ]
  ezAnimGraphNumberInputPin m_InCoordY;                       // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose;                    // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnStarted;                 // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFinished;                // [ property ]

  struct ClipToPlay
  {
    EZ_DECLARE_POD_TYPE();

    ezUInt32 m_uiIndex;
    float m_fWeight = 1.0f;
    const ezAnimController::AnimClipInfo* m_pClipInfo = nullptr;
  };

  struct InstanceState
  {
    ezTime m_CenterPlaybackTime = ezTime::MakeFromHours(1000);
    float m_fOtherPlaybackPosNorm = 0.0f;
    float m_fLastValueX = 0.0f;
    float m_fLastValueY = 0.0f;
  };

  void UpdateCenterClipPlaybackTime(const ezAnimController::AnimClipInfo& centerInfo, InstanceState* pState, ezAnimGraphInstance& ref_graph, ezTime tDiff, ezAnimPoseEventTrackSampleMode& out_eventSamplingCenter) const;
  void PlayClips(ezAnimController& ref_controller, const ezAnimController::AnimClipInfo& centerInfo, InstanceState* pState, ezAnimGraphInstance& ref_graph, ezTime tDiff, ezArrayPtr<ClipToPlay> clips, ezUInt32 uiMaxWeightClip) const;
  void ComputeClipsAndWeights(ezAnimController& ref_controller, const ezAnimController::AnimClipInfo& centerInfo, const ezVec2& p, ezDynamicArray<ClipToPlay>& out_Clips, ezUInt32& out_uiMaxWeightClip) const;
};
