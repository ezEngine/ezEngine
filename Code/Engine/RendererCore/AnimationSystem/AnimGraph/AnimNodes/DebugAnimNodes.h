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

  //////////////////////////////////////////////////////////////////////////
  // ezLogAnimNode

protected:
  ezString m_sText;                        // [ property ]
  ezAnimGraphTriggerInputPin m_InActivate; // [ property ]
  ezAnimGraphNumberInputPin m_InNumber0;   // [ property ]
  ezAnimGraphNumberInputPin m_InNumber1;   // [ property ]
  ezAnimGraphNumberInputPin m_InNumber2;   // [ property ]
  ezAnimGraphNumberInputPin m_InNumber3;   // [ property ]
};

class EZ_RENDERERCORE_DLL ezLogInfoAnimNode : public ezLogAnimNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogInfoAnimNode, ezLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // ezLogAnimNode

protected:
  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
};

class EZ_RENDERERCORE_DLL ezLogErrorAnimNode : public ezLogAnimNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezLogErrorAnimNode, ezLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // ezLogAnimNode

protected:
  virtual void Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
};
