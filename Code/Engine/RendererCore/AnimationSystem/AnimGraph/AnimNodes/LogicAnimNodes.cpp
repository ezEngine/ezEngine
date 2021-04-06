#include <RendererCorePCH.h>

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
    new ezColorAttribute(ezColor::DarkOliveGreen),
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
    new ezColorAttribute(ezColor::DarkOliveGreen),
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
    new ezColorAttribute(ezColor::DarkOliveGreen),
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
