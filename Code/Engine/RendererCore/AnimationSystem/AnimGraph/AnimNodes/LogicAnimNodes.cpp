#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LogicAnimNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogicAndAnimNode, 1, ezRTTIDefaultAllocator<ezLogicAndAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("NegateResult", m_bNegateResult),
    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("AND"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLogicAndAnimNode::ezLogicAndAnimNode() = default;
ezLogicAndAnimNode::~ezLogicAndAnimNode() = default;

ezResult ezLogicAndAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_bNegateResult;
  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogicAndAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_bNegateResult;
  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogicAndAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  bool res = m_ActivePin.AreAllTriggered(graph);

  if (m_bNegateResult)
  {
    res = !res;
  }

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogicOrAnimNode, 1, ezRTTIDefaultAllocator<ezLogicOrAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("NegateResult", m_bNegateResult),
    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("OR"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLogicOrAnimNode::ezLogicOrAnimNode() = default;
ezLogicOrAnimNode::~ezLogicOrAnimNode() = default;

ezResult ezLogicOrAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_bNegateResult;
  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogicOrAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_bNegateResult;
  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogicOrAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  bool res = m_ActivePin.IsTriggered(graph);

  if (m_bNegateResult)
  {
    res = !res;
  }

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogicNotAnimNode, 1, ezRTTIDefaultAllocator<ezLogicNotAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("Output", m_OutputPin)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("NOT"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLogicNotAnimNode::ezLogicNotAnimNode() = default;
ezLogicNotAnimNode::~ezLogicNotAnimNode() = default;

ezResult ezLogicNotAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutputPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogicNotAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutputPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogicNotAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  bool res = !m_ActivePin.IsTriggered(graph);

  m_OutputPin.SetTriggered(graph, res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezCompareNumberAnimNode, 1, ezRTTIDefaultAllocator<ezCompareNumberAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("ReferenceValue", m_fReferenceValue)->AddAttributes(new ezDefaultValueAttribute(1.0f)),
    EZ_ENUM_MEMBER_PROPERTY("Comparison", ezComparisonOperator, m_Comparison),

    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
    EZ_MEMBER_PROPERTY("Number", m_NumberPin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("Check: Number {Comparison} {ReferenceValue}"),
    new ezColorAttribute(ezColorScheme::GetColorForUI(ezColorScheme::Lime)),
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

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_NumberPin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezCompareNumberAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_fReferenceValue;
  stream >> m_Comparison;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_NumberPin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezCompareNumberAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (ezComparisonOperator::Compare(m_Comparison, m_NumberPin.GetNumber(graph), m_fReferenceValue))
  {
    m_ActivePin.SetTriggered(graph, true);
  }
  else
  {
    m_ActivePin.SetTriggered(graph, false);
  }
}
