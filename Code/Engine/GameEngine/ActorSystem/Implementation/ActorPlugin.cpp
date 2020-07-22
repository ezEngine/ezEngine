#include <GameEnginePCH.h>

#include <GameEngine/ActorSystem/ActorPlugin.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActorPlugin, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezActorPlugin::ezActorPlugin() = default;
ezActorPlugin::~ezActorPlugin() = default;

ezActor* ezActorPlugin::GetActor() const
{
  return m_pOwningActor;
}


EZ_STATICLINK_FILE(GameEngine, GameEngine_ActorSystem_Implementation_ActorPlugin);
