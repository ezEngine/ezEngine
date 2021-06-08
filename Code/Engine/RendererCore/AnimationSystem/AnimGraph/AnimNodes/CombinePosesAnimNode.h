#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezCombinePosesAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCombinePosesAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLocalToModelPoseAnimNode

public:
  ezCombinePosesAnimNode();
  ~ezCombinePosesAnimNode();

  ezUInt8 m_uiMaxPoses = 8; // [ property ]

private:
  ezAnimGraphLocalPoseMultiInputPin m_LocalPosesPin; // [ property ]
  ezAnimGraphLocalPoseOutputPin m_LocalPosePin;      // [ property ]

  ezDynamicArray<ozz::math::SimdFloat4, ezAlignedAllocatorWrapper> m_BlendMask;
};
