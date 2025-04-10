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

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezCompareNumberAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezCompareNumberAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezCompareNumberAnimNode

public:
  double m_fReferenceValue = 0.0f;           // [ property ]
  ezEnum<ezComparisonOperator> m_Comparison; // [ property ]

private:
  ezAnimGraphNumberInputPin m_InNumber;      // [ property ]
  ezAnimGraphNumberInputPin m_InReference;   // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsTrue;      // [ property ]
  ezAnimGraphBoolOutputPin m_OutIsFalse;     // [ property ]
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezBoolToNumberAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoolToNumberAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezBoolToNumberAnimNode

public:
  ezBoolToNumberAnimNode();
  ~ezBoolToNumberAnimNode();

  double m_fFalseValue = 0.0f;
  double m_fTrueValue = 1.0f;

private:
  ezAnimGraphBoolInputPin m_InValue;      // [ property ]
  ezAnimGraphNumberOutputPin m_OutNumber; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class EZ_RENDERERCORE_DLL ezBoolToTriggerAnimNode : public ezAnimGraphNode
{
  EZ_ADD_DYNAMIC_REFLECTION(ezBoolToTriggerAnimNode, ezAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // ezAnimGraphNode

protected:
  virtual ezResult SerializeNode(ezStreamWriter& stream) const override;
  virtual ezResult DeserializeNode(ezStreamReader& stream) override;

  virtual void Step(ezAnimController& ref_controller, ezAnimGraphInstance& ref_graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // ezBoolToNumberAnimNode

public:
  ezBoolToTriggerAnimNode();
  ~ezBoolToTriggerAnimNode();

private:
  ezAnimGraphBoolInputPin m_InValue;        // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnTrue;  // [ property ]
  ezAnimGraphTriggerOutputPin m_OutOnFalse; // [ property ]

  struct InstanceData
  {
    ezInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};
