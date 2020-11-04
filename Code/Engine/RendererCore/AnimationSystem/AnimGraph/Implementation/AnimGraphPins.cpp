#include <RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphPins.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphPin, 1, ezRTTIDefaultAllocator<ezAnimGraphPin>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("PinIdx", m_iPinIndex)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphTriggerInputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphTriggerInputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAnimGraphTriggerOutputPin, 1, ezRTTIDefaultAllocator<ezAnimGraphTriggerOutputPin>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezResult ezAnimGraphPin::Serialize(ezStreamWriter& stream) const
{
  stream << m_iPinIndex;
  return EZ_SUCCESS;
}

ezResult ezAnimGraphPin::Deserialize(ezStreamReader& stream)
{
  stream >> m_iPinIndex;
  return EZ_SUCCESS;
}

void ezAnimGraphTriggerOutputPin::SetTriggered(ezAnimGraph& controller, bool triggered)
{
  if (m_iPinIndex < 0)
    return;

  if (m_bTriggered == triggered)
    return;

  m_bTriggered = triggered;

  const auto& map = controller.m_TriggerOutputToInputPinMapping[m_iPinIndex];

  const ezInt8 offset = triggered ? +1 : -1;

  // trigger or reset all input pins that are connected to this output pin
  for (ezUInt16 idx : map)
  {
    controller.m_TriggerInputPinStates[idx] += offset;
  }
}

bool ezAnimGraphTriggerInputPin::IsTriggered(ezAnimGraph& controller) const
{
  if (m_iPinIndex < 0)
    return false;

  return controller.m_TriggerInputPinStates[m_iPinIndex] > 0;
}
