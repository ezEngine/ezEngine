#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/AgentSteeringComponent.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezAgentSteeringComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("AI/Experimental"),
    new ezInDevelopmentAttribute(ezInDevelopmentAttribute::Phase::Alpha),
  }
  EZ_END_ATTRIBUTES;
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(SetTargetPosition, In, "position"),
    EZ_SCRIPT_FUNCTION_PROPERTY(GetTargetPosition),
    EZ_SCRIPT_FUNCTION_PROPERTY(ClearTargetPosition),
    //EZ_SCRIPT_FUNCTION_PROPERTY(GetPathToTargetState),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezAgentSteeringComponent::ezAgentSteeringComponent() = default;
ezAgentSteeringComponent::~ezAgentSteeringComponent() = default;

void ezAgentSteeringComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();
}

void ezAgentSteeringComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();
}



EZ_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_AgentSteeringComponent);
