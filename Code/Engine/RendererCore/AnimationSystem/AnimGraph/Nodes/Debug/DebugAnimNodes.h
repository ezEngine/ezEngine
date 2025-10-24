#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

/// Base class for logging nodes that output debugging information.
///
/// This node logs text and number values to the console when triggered. Use derived classes
/// (ezLogInfoAnimNode, ezLogErrorAnimNode) for different log levels. Useful for debugging animation
/// state, tracking blend weights, or monitoring node execution.
class EZ_RENDERERCORE_DLL ezLogAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogAnimNode

protected:
  ezString m_sText;                                        // [ property ]
  ezAnimGraphTriggerInputPin m_InActivate;                 // [ property ]
  ezUInt8 m_uiNumberCount = 1;                             // [ property ]
  ezHybridArray<ezAnimGraphNumberInputPin, 2> m_InNumbers; // [ property ]
};

/// Logs informational messages to the console.
class EZ_RENDERERCORE_DLL ezLogInfoAnimNode : public ezLogAnimNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogInfoAnimNode, ezLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // ezLogAnimNode

protected:
  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
};

/// Logs error messages to the console.
class EZ_RENDERERCORE_DLL ezLogErrorAnimNode : public ezLogAnimNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogErrorAnimNode, ezLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // ezLogAnimNode

protected:
  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
};
