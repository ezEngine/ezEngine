#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezSwitchPoseAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSwitchPoseAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSelectPoseAnimNode

private:
  ezTime m_TransitionDuration = ezTime::MakeFromMilliseconds(200); // [ property ]
  ezUInt8 m_uiPosesCount = 0;                                      // [ property ]
  ezHybridArray<ezAnimGraphLocalPoseInputPin, 4> m_InPoses;        // [ property ]
  ezAnimGraphNumberInputPin m_InIndex;                             // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose;                         // [ property ]

  struct InstanceData
  {
    ezTime m_TransitionTime;
    ezInt8 m_iTransitionFromIndex = -1;
    ezInt8 m_iTransitionToIndex = -1;
  };
};
