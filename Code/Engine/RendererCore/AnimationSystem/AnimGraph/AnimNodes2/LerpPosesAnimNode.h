#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezLerpPosesAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLerpPosesAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLerpPosesAnimNode

public:
  ezLerpPosesAnimNode();
  ~ezLerpPosesAnimNode();

  float m_fLerp = 0.5f; // [ property ]

private:
  ezAnimGraphNumberInputPin m_InLerp;      // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose0;  // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose1;  // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose2;  // [ property ]
  ezAnimGraphLocalPoseInputPin m_InPose3;  // [ property ]
  ezAnimGraphLocalPoseOutputPin m_OutPose; // [ property ]
};
