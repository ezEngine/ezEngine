#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiInputPin.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiInputPin, 1, ezRTTIDefaultAllocator<ezAiInputPin>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Connections", m_Connections),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAiInputPinAttribute, 1, ezRTTIDefaultAllocator<ezAiInputPinAttribute>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("NodeType", m_sNodeType),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_CONSTRUCTOR_PROPERTY(const char*),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const ezRTTI* ezAiInputPinAttribute::GetNodeType() const
{
  return ezRTTI::FindTypeByName(m_sNodeType);
}

bool ezAiInputPinAttribute::Accepts(const ezRTTI* pNodeType) const
{
  const ezRTTI* pAccepted = GetNodeType();
  if (pAccepted == nullptr || pNodeType == nullptr)
    return false;

  if (pAccepted->IsDerivedFrom<ezAiPerception>())
  {
    if (!pNodeType->IsDerivedFrom<ezAiPerceptionGenerator>())
      return false;

    const ezRTTI* pProduced = ezAiPerceptionGenerator::GetPerceptionType(pNodeType);
    return pProduced != nullptr && pProduced->IsDerivedFrom(pAccepted);
  }

  return pNodeType->IsDerivedFrom(pAccepted);
}
