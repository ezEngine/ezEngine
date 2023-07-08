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

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicAndAnimNode

public:
  ezLogicAndAnimNode();
  ~ezLogicAndAnimNode();

private:
  ezAnimGraphBoolInputPin m_InBool0;     // [ property ]
  ezAnimGraphBoolInputPin m_InBool1;     // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsTrue;  // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsFalse; // [ property ]
};

class EZ_RENDERERCORE_DLL ezLogicEventAndAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogicEventAndAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicEventAndAnimNode

public:
  ezLogicEventAndAnimNode();
  ~ezLogicEventAndAnimNode();

private:
  ezAnimGraphTriggerInputPin m_InActivate;      // [ property ]
  ezAnimGraphBoolInputPin m_InBool;             // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnActivated; // [ property ]
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

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicOrAnimNode

public:
  ezLogicOrAnimNode();
  ~ezLogicOrAnimNode();

private:
  ezAnimGraphBoolInputPin m_InBool0;     // [ property ]
  ezAnimGraphBoolInputPin m_InBool1;     // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsTrue;  // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsFalse; // [ property ]
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

  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicNotAnimNode

public:
  ezLogicNotAnimNode();
  ~ezLogicNotAnimNode();

private:
  ezAnimGraphBoolInputPin m_InBool;   // [ property ]
  ezAnimGraphBoolOutputPin m_OutBool; // [ property ]
};

