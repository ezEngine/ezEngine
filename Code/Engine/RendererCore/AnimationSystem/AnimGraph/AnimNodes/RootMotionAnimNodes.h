#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezRootRotationAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezRootRotationAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezRootRotationAnimNode

public:
  ezRootRotationAnimNode();
  ~ezRootRotationAnimNode();

private:
  ezAnimGraphNumberInputPin m_InRotateX; // [ property ]
  ezAnimGraphNumberInputPin m_InRotateY; // [ property ]
  ezAnimGraphNumberInputPin m_InRotateZ; // [ property ]
};
