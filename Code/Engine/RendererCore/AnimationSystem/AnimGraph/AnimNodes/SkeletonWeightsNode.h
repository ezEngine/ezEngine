#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class ezSkeletonResource;
class ezStreamWriter;
class ezStreamReader;

class EZ_RENDERERCORE_DLL ezSkeletonWeightsAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezSkeletonWeightsAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph* pOwner, ezTime tDiff, const ezSkeletonResource* pSkeleton) override;

  //////////////////////////////////////////////////////////////////////////
  // ezSkeletonWeightsAnimNode

public:
  ezSkeletonWeightsAnimNode();
  ~ezSkeletonWeightsAnimNode();

  float m_fWeight = 10.0f; // [ property ]

  void SetPartialBlendingRootBone(const char* szBone); // [ property ]
  const char* GetPartialBlendingRootBone() const;      // [ property ]

private:
  ezAnimGraphSkeletonWeightsOutputPin m_Weights; // [ property ]

  ezHashedString m_sPartialBlendingRootBone;

  ezAnimGraphBlendWeights* m_pPartialBlendingMask = nullptr;
};
