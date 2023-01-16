#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/EventAnimNode.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezEventAnimNode, 1, ezRTTIDefaultAllocator<ezEventAnimNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("EventName", GetEventName, SetEventName),

    EZ_MEMBER_PROPERTY("Active", m_ActivePin)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Events"),
    new ezColorAttribute(ezColorScheme::DarkUI(ezColorScheme::Orange)),
    new ezTitleAttribute("Event: '{EventName}'"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezEventAnimNode::SerializeNode(ezStreamWriter& stream) const
{
  stream.WriteVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sEventName;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Serialize(stream));

  return EZ_SUCCESS;
}

ezResult ezEventAnimNode::DeserializeNode(ezStreamReader& stream)
{
  stream.ReadVersion(1);

  EZ_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sEventName;

  EZ_SUCCEED_OR_RETURN(m_ActivePin.Deserialize(stream));

  return EZ_SUCCESS;
}

void ezEventAnimNode::Step(ezAnimGraph& graph, ezTime tDiff, const ezSkeletonResource* pSkeleton, ezGameObject* pTarget)
{
  if (!m_ActivePin.IsConnected() || m_sEventName.IsEmpty())
    return;

  if (m_ActivePin.IsTriggered(graph))
  {
    ezMsgGenericEvent msg;
    msg.m_sMessage = m_sEventName;
    msg.m_Value = ezVariant();

    pTarget->SendEventMessage(msg, nullptr);
  }
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_EventAnimNode);
