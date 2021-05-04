#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezLogAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogAnimNode

public:
  ezString m_sText;

private:
  ezAnimGraphTriggerInputPin m_ActivePin; // [ property ]
  ezAnimGraphTriggerInputPin m_Input0;    // [ property ]
  ezAnimGraphTriggerInputPin m_Input1;    // [ property ]
  ezAnimGraphNumberInputPin m_Input2;     // [ property ]
  ezAnimGraphNumberInputPin m_Input3;     // [ property ]
};
