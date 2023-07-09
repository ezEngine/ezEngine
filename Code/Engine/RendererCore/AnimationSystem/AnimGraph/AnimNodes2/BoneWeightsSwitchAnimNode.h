#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezSwitchBoneWeightsAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSwitchBoneWeightsAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSwitchBoneWeightsAnimNode

private:
  ezAnimGraphNumberInputPin m_InIndex;          // [ property ]
  ezAnimGraphBoneWeightsInputPin m_InWeights0;  // [ property ]
  ezAnimGraphBoneWeightsInputPin m_InWeights1;  // [ property ]
  ezAnimGraphBoneWeightsInputPin m_InWeights2;  // [ property ]
  ezAnimGraphBoneWeightsInputPin m_InWeights3;  // [ property ]
  ezAnimGraphBoneWeightsOutputPin m_OutWeights; // [ property ]
};
