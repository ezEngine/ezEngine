#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>

struct EZ_RENDERERCORE_DLL ezAnimationClip1D
{
  ezHashedString m_sClip;
  float m_fPosition = 0.0f;
  float m_fSpeed = 1.0f;

  void SetAnimationFile(const char* szFile);
  const char* GetAnimationFile() const;
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_RENDERERCORE_DLL, ezAnimationClip1D);

class EZ_RENDERERCORE_DLL ezSampleBlendSpace1DAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSampleBlendSpace1DAnimNode, ezAnimGraphNode);

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
  ezSampleBlendSpace1DAnimNode();
  ~ezSampleBlendSpace1DAnimNode();

private:
  ezHybridArray<ezAnimationClip1D, 4> m_Clips; // [ property ]
  bool m_bLoop = true;                         // [ property ]
  float m_fRootMotionAmount = 0.0f;            // [ property ]
  float m_fPlaybackSpeed = 1.0f;               // [ property ]

  ezAnimGraphTriggerInputPin m_InStart;        // [ property ]
  ezAnimGraphBoolInputPin m_InLoop;            // [ property ]
  ezAnimGraphNumberInputPin m_InSpeed;         // [ property ]
  ezAnimGraphNumberInputPin m_InLerp;          // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose;     // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnStarted;  // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFinished; // [ property ]


  struct InstanceState
  {
    ezTime m_PlaybackTime = ezTime::MakeFromHours(1000);
  };
};
