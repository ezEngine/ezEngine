#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Boolean AND logic node that outputs true only when all inputs are true.
///
/// This node combines multiple boolean inputs using AND logic. Useful for combining multiple
/// conditions before triggering animations or state transitions.
class EZ_RENDERERCORE_DLL ezLogicAndAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogicAndAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicAndAnimNode

public:
  ezLogicAndAnimNode();
  ~ezLogicAndAnimNode();

private:
  ezUInt8 m_uiBoolCount = 2;                          // [ property ]
  ezHybridArray<ezAnimGraphBoolInputPin, 2> m_InBool; // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsTrue;               // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsFalse;              // [ property ]
};

/// Forwards a trigger event only when a boolean condition is true.
///
/// This node gates trigger events based on a boolean input. The trigger is only forwarded
/// when the boolean condition is satisfied. Useful for conditional event routing.
class EZ_RENDERERCORE_DLL ezLogicEventAndAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogicEventAndAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

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

/// Boolean OR logic node that outputs true when any input is true.
///
/// This node combines multiple boolean inputs using OR logic. Useful for triggering animations
/// when any of several conditions are met.
class EZ_RENDERERCORE_DLL ezLogicOrAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogicOrAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicOrAnimNode

public:
  ezLogicOrAnimNode();
  ~ezLogicOrAnimNode();

private:
  ezUInt8 m_uiBoolCount = 2;                          // [ property ]
  ezHybridArray<ezAnimGraphBoolInputPin, 2> m_InBool; // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsTrue;               // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsFalse;              // [ property ]
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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicNotAnimNode

public:
  ezLogicNotAnimNode();
  ~ezLogicNotAnimNode();

private:
  ezAnimGraphBoolInputPin m_InBool;   // [ property ]
  ezAnimGraphBoolOutputPin m_OutBool; // [ property ]
};
