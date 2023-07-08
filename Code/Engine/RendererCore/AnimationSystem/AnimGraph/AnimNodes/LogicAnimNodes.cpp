#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/LogicAnimNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogicAndAnimNode, 1, ezRTTIDefaultAllocator<ezLogicAndAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InBool0", m_InBool0)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("InBool1", m_InBool1)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new ezHiddenAttribute),
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

  EZ_SUCCEED_OR_RETURN(m_InBool0.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool1.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogicAndAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InBool0.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool1.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogicAndAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  bool res = m_InBool0.GetBool(graph, true) && m_InBool1.GetBool(graph, true);

  m_OutIsTrue.SetBool(graph, res);
  m_OutIsFalse.SetBool(graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogicEventAndAnimNode, 1, ezRTTIDefaultAllocator<ezLogicEventAndAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutOnActivated", m_OutOnActivated)->AddAttributes(new ezHiddenAttribute),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Logic"),
    new ezTitleAttribute("Event AND"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezLogicEventAndAnimNode::ezLogicEventAndAnimNode() = default;
ezLogicEventAndAnimNode::~ezLogicEventAndAnimNode() = default;

ezResult ezLogicEventAndAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnActivated.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogicEventAndAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutOnActivated.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogicEventAndAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (m_InActivate.IsTriggered(graph) && m_InBool.GetBool(graph))
  {
    m_OutOnActivated.SetTriggered(graph);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogicOrAnimNode, 1, ezRTTIDefaultAllocator<ezLogicOrAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InBool0", m_InBool0)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("InBool1", m_InBool1)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutIsTrue", m_OutIsTrue)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutIsFalse", m_OutIsFalse)->AddAttributes(new ezHiddenAttribute),
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

  EZ_SUCCEED_OR_RETURN(m_InBool0.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool1.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsFalse.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogicOrAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InBool0.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InBool1.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsTrue.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutIsFalse.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogicOrAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  bool res = m_InBool0.GetBool(graph, false) || m_InBool1.GetBool(graph, false);

  m_OutIsTrue.SetBool(graph, res);
  m_OutIsFalse.SetBool(graph, !res);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogicNotAnimNode, 1, ezRTTIDefaultAllocator<ezLogicNotAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("InBool", m_InBool)->AddAttributes(new ezHiddenAttribute),
    EZ_MEMBER_PROPERTY("OutBool", m_OutBool)->AddAttributes(new ezHiddenAttribute),
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

  EZ_SUCCEED_OR_RETURN(m_InBool.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutBool.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogicNotAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  EZ_SUCCEED_OR_RETURN(m_InBool.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_OutBool.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogicNotAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  const bool value = !m_InBool.GetBool(graph);

  m_OutBool.SetBool(graph, !value);
}

EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_LogicAnimNodes);
