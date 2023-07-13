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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezSwitchBoneWeightsAnimNode

private:
  ezAnimGraphNumberInputPin m_InIndex;                          // [ property ]
  ezUInt8 m_uiWeightsCount = 0;                                 // [ property ]
  ezHybridArray<ezAnimGraphBoneWeightsInputPin, 2> m_InWeights; // [ property ]
  ezAnimGraphBoneWeightsOutputPin m_OutWeights;                 // [ property ]
};
