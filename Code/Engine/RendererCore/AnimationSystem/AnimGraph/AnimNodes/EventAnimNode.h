#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezEventAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezEventAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezEventAnimNode

public:
  void SetEventName(const char* szSz) { m_sEventName.Assign(szSz); }
  const char* GetEventName() const { return m_sEventName.GetString(); }

private:
  ezHashedString m_sEventName;            // [ property ]
  ezAnimGraphTriggerInputPin m_ActivePin; // [ property ]
};
