#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezLogicAndAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogicAndAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicAndAnimNode

public:
  ezLogicAndAnimNode();
  ~ezLogicAndAnimNode();

  bool m_bNegateResult = false; // [ property ]

private:
  ezAnimGraphTriggerInputPin m_ActivePin;  // [ property ]
  ezAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezLogicOrAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogicOrAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicOrAnimNode

public:
  ezLogicOrAnimNode();
  ~ezLogicOrAnimNode();

  bool m_bNegateResult = false; // [ property ]

private:
  ezAnimGraphTriggerInputPin m_ActivePin;  // [ property ]
  ezAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezLogicNotAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogicNotAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicNotAnimNode

public:
  ezLogicNotAnimNode();
  ~ezLogicNotAnimNode();

private:
  ezAnimGraphTriggerInputPin m_ActivePin;  // [ property ]
  ezAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};
