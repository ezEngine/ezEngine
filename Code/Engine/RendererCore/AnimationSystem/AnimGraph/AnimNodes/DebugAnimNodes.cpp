#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/DebugAnimNodes.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezLogAnimNode, 1, ezRTTIDefaultAllocator<ezLogAnimNode>)
  {
    EZ_BEGIN_PROPERTIES
    {
      EZ_MEMBER_PROPERTY("Text", m_sText),

      EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Input0", m_Input0)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Input1", m_Input1)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Input2", m_Input2)->AddAttributes(new ezHiddenAttribute()),
      EZ_MEMBER_PROPERTY("Input3", m_Input3)->AddAttributes(new ezHiddenAttribute()),
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

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input0.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input1.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input2.Serialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input3.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezLogAnimNode::DeserializeNode(ezStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input0.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input1.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input2.Deserialize(stream));
  EZ_SUCCEED_OR_RETURN(m_Input3.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezLogAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_ActivePin.IsTriggered(graph))
    return;

  ezLog::Dev(m_sText, m_Input0.IsTriggered(graph), m_Input1.IsTriggered(graph), m_Input2.GetNumber(graph), m_Input3.GetNumber(graph));
}
