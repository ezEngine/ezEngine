#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class EZ_RENDERERCORE_DLL ezMathExpressionAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezMathExpressionAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Initialize(ezAnimGraph& graph, const ezSkeletonResource* pSkeleton) override;
  virtual void Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezLogicAndAnimNode

public:
  ezMathExpressionAnimNode();
  ~ezMathExpressionAnimNode();

  void SetExpression(ezString sExpr);
  ezString GetExpression() const;

private:
  ezAnimGraphNumberInputPin m_ValueAPin;  // [ property ]
  ezAnimGraphNumberInputPin m_ValueBPin;  // [ property ]
  ezAnimGraphNumberInputPin m_ValueCPin;  // [ property ]
  ezAnimGraphNumberInputPin m_ValueDPin;  // [ property ]
  ezAnimGraphNumberOutputPin m_ResultPin; // [ property ]

  ezString m_sExpression;

  struct InstanceData
  {
    ezMathExpression m_mExpression;
  };
};
