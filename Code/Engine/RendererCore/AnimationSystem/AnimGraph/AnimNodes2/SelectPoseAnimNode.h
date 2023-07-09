#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezSelectPoseAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSelectPoseAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSelectPoseAnimNode

private:
  ezTime m_TransitionDuration = ezTime::Milliseconds(200); // [ property ]

  ezAnimGraphNumberInputPin m_InIndex;     // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose0;  // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose1;  // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose2;  // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose3;  // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose; // [ property ]

  struct InstanceData
  {
    ezTime m_TransitionTime;
    ezInt8 m_iTransitionFromIndex = -1;
    ezInt8 m_iTransitionToIndex = -1;
  };
};
