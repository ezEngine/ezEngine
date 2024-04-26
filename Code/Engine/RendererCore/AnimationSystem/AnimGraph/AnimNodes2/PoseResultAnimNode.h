#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezPoseResultAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPoseResultAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezPoseResultAnimNode

public:
  ezPoseResultAnimNode();
  ~ezPoseResultAnimNode();

private:
  ezTime m_FadeDuration = ezTime::MakeFromMilliseconds(200); // [ property ]

  ezAnimGraphLocalPoseInputPin m_InPose;                     // [ property ]
  ezAnimGraphNumberInputPin m_InTargetWeight;                // [ property ]
  ezAnimGraphNumberInputPin m_InFadeDuration;                // [ property ]
  ezAnimGraphBoneWeightsInputPin m_InWeights;                // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFadedOut;               // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFadedIn;                // [ property ]
  ezAnimGraphNumberOutputPin m_OutCurrentWeight;             // [ property ]

  struct InstanceData
  {
    float m_fStartWeight = 0.0f;
    float m_fEndWeight = 0.0f;
    ezTime m_PlayTime = ezTime::MakeZero();
    ezTime m_EndTime = ezTime::MakeZero();
  };
};
