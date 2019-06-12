#include <CorePCH.h>

#include <Core/Actor/Actor.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezActor, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

ezEvent<const ezActorEvent&> ezActor::s_Events;


ezActorManager* ezActor::GetManager() const
{
  return m_pManager;
}

const char* ezActor::GetName() const
{
  return m_sName;
}

ezActor::ezActor(const char* szActorName)
  : m_sName(szActorName)
{
}

ezActor::~ezActor() = default;

void ezActor::Activate() {}
void ezActor::Deactivate() {}
void ezActor::Update() {}
