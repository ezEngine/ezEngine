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

void ezMathExpressionAnimNode::SetExpression(const char* szSz)
{
  m_mExpression.Reset(szSz);
}

const char* ezMathExpressionAnimNode::GetExpression() const
{
  return m_mExpression.GetExpressionString();
}

ezResult ezMathExpressionAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_mExpression.GetExpressionString();
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

  ezStringBuilder tmp;
  stream >> tmp;
  m_mExpression.Reset(tmp);
  EZ_SUCCEED_OR_RETURN(m_ValueAPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueBPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueCPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ValueDPin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_ResultPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezMathExpressionAnimNode::Initialize(ezAnimGraph& graph, const ezSkeletonResource* pSkeleton)
{
  if (!m_mExpression.IsValid() && m_ResultPin.IsConnected())
  {
    ezLog::Error("Math expression '{}' is invalid.", m_mExpression.GetExpressionString());
  }
}

static ezHashedString s_sA = ezMakeHashedString("a");
static ezHashedString s_sB = ezMakeHashedString("b");
static ezHashedString s_sC = ezMakeHashedString("c");
static ezHashedString s_sD = ezMakeHashedString("d");

void ezMathExpressionAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_mExpression.IsValid())
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

  float result = m_mExpression.Evaluate(inputs);
  m_ResultPin.SetNumber(graph, result);
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_MathAnimNodes);
