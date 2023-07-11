#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/MathAnimNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezMathExpressionAnimNode, 1, ezRTTIDefaultAllocator<ezMathExpressionAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Expression", GetExpression, SetExpression)->AddAttributes(new ezDefaultValueAttribute("a*a + (b-c) / abs(d)")),
    EZ_MEMBER_PROPERTY("a", m_ValueAPin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("b", m_ValueBPin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("c", m_ValueCPin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("d", m_ValueDPin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("Result", m_ResultPin)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Math"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Lime)),
    new ezTitleAttribute("= {Expression}"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezMathExpressionAnimNode::ezMathExpressionAnimNode() = default;
ezMathExpressionAnimNode::~ezMathExpressionAnimNode() = default;

void ezMathExpressionAnimNode::SetExpression(ezString sExpr)
{
  m_sExpression = sExpr;
}

ezString ezMathExpressionAnimNode::GetExpression() const
{
  return m_sExpression;
}

ezResult ezMathExpressionAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sExpression;
  EZ_SUCCEED_OR_RETURN(m_ValueAPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueBPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueCPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueDPin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ResultPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezMathExpressionAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sExpression;
  EZ_SUCCEED_OR_RETURN(m_ValueAPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueBPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueCPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueDPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ResultPin.Deserialize(stream));

  return EZ_SUCCESS;
}

static ezHashedString s_sA = ezMakeHashedString("a");
static ezHashedString s_sB = ezMakeHashedString("b");
static ezHashedString s_sC = ezMakeHashedString("c");
static ezHashedString s_sD = ezMakeHashedString("d");

void ezMathExpressionAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  InstanceData* pInstance = graph.GetAnimNodeInstanceData<InstanceData>(*this);

  if (pInstance->m_mExpression.GetExpressionString().IsEmpty())
  {
    pInstance->m_mExpression.Reset(m_sExpression);
  }

  if (!pInstance->m_mExpression.IsValid())
  {
    m_ResultPin.SetNumber(graph, 0);
    return;
  }

  ezMathExpression::Input inputs[] =
    {
      {s_sA, static_cast<float>(m_ValueAPin.GetNumber(graph))},
      {s_sB, static_cast<float>(m_ValueBPin.GetNumber(graph))},
      {s_sC, static_cast<float>(m_ValueCPin.GetNumber(graph))},
      {s_sD, static_cast<float>(m_ValueDPin.GetNumber(graph))},
    };

  float result = pInstance->m_mExpression.Evaluate(inputs);
  m_ResultPin.SetNumber(graph, result);
}

bool ezMathExpressionAnimNode::GetInstanceDataDesc(ezInstanceDataDesc& out_desc) const
{
  out_desc.FillFromType<InstanceData>();
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCompareNumberAnimNode, 1, ezRTTIDefaultAllocator<ezCompareNumberAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue),
    EZ_ENUM_MEMBER_PROPERTY("Comparison", ezComparisonOperator, m_Comparison),

    EZ_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InNumber", m_InNumber)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("InReference", m_InReference)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("Compare: Number {Comparison} {ReferenceValue}"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Lime)),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezCompareNumberAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fReferenceValue;
  stream << m_Comparison;

  EZ_SUCCEED_OR_RETURN(m_InNumber.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InReference.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezCompareNumberAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  EZ_SUCCEED_OR_RETURN(m_InNumber.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InReference.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezCompareNumberAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  const bool bIsTrue = ezComparisonOperator::Compare<double>(m_Comparison, m_InNumber.GetNumber(graph), m_InReference.GetNumber(graph, m_fReferenceValue));

  m_OutIsTrue.SetBool(graph, bIsTrue);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezBoolToNumberAnimNode, 1, ezRTTIDefaultAllocator<ezBoolToNumberAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("FalseValue", m_fFalseValue)->AddAttributes(new ezDefaultValueAttribute(0.0)),
    EZ_MEMBER_PROPERTY("TrueValue", m_fTrueValue)->AddAttributes(new ezDefaultValueAttribute(1.0)),
    EZ_MEMBER_PROPERTY("InValue", m_InValue)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutNumber", m_OutNumber)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("Bool To Number"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezBoolToNumberAnimNode::ezBoolToNumberAnimNode() = default;
ezBoolToNumberAnimNode::~ezBoolToNumberAnimNode() = default;

ezResult ezBoolToNumberAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_fFalseValue;
  stream << m_fTrueValue;

  EZ_SUCCEED_OR_RETURN(m_InValue.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutNumber.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezBoolToNumberAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fFalseValue;
  stream >> m_fTrueValue;

  EZ_SUCCEED_OR_RETURN(m_InValue.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutNumber.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezBoolToNumberAnimNode::Step(ezAnimController& ref_controller, ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  m_OutNumber.SetNumber(graph, m_InValue.GetBool(graph) ? m_fTrueValue : m_fFalseValue);
}



EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);
