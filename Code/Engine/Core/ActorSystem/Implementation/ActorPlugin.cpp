#include <CorePCH.h>

#include <Core/ActorSystem/ActorPlugin.h>

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


EZ_STATICLINK_FILE(Core, Core_ActorSystem_Implementation_ActorPlugin);
