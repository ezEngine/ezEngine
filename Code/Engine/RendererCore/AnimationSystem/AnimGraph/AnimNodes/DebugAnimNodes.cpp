#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/DebugAnimNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogAnimNode, 1, ezRTTINoAllocator)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new ezDefaultValueAttribute("Values: {0}/{1}-{3}/{4}")),

      EZ_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InNumber0", m_InNumber0)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InNumber1", m_InNumber1)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InNumber2", m_InNumber2)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("InNumber3", m_InNumber3)->AddAttributes(new ezHiddenAttribute()),
    }
    EZ_END_PROPERTIES;
    EZ_BEGIN_ATTRIBUTES
    {
      new ezCategoryAttribute("Debug"),
      new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Pink)),
      new ezTitleAttribute("Log: '{Text}'"),
    }
    EZ_END_ATTRIBUTES;
  }
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezLogAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sText;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber0.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber1.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber2.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber3.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  EZ_IGNORE_UNUSED(version);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;

  EZ_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber0.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber1.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber2.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_InNumber3.Deserialize(stream));

  return EZ_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogInfoAnimNode, 1, ezRTTIDefaultAllocator<ezLogInfoAnimNode>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Log Info: '{Text}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLogInfoAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(graph))
    return;

  ezLog::Info(m_sText, m_InNumber0.GetNumber(graph), m_InNumber1.GetNumber(graph), m_InNumber2.GetNumber(graph), m_InNumber3.GetNumber(graph));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogErrorAnimNode, 1, ezRTTIDefaultAllocator<ezLogErrorAnimNode>)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezTitleAttribute("Log Error: '{Text}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void ezLogErrorAnimNode::Step(ezAnimGraphInstance& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(graph))
    return;

  ezLog::Error(m_sText, m_InNumber0.GetNumber(graph), m_InNumber1.GetNumber(graph), m_InNumber2.GetNumber(graph), m_InNumber3.GetNumber(graph));
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_DebugAnimNodes);
