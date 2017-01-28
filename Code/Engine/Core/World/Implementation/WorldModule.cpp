#include <Core/PCH.h>
#include <Core/World/World.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWorldModule, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE

static ezUInt16 s_uiNextTypeId = 0;

ezWorldModule::ezWorldModule(ezWorld* pWorld)
  : m_pWorld(pWorld)
{

}

ezWorldModule::~ezWorldModule()
{
}

// protected methods

void ezWorldModule::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void ezWorldModule::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}

ezAllocatorBase* ezWorldModule::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

ezInternal::WorldLargeBlockAllocator* ezWorldModule::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

bool ezWorldModule::GetWorldSimulationEnabled() const
{
  return m_pWorld->GetWorldSimulationEnabled();
}

ezUInt16 ezWorldModule::GetNextTypeId()
{
  return s_uiNextTypeId++;
}




EZ_STATICLINK_FILE(Core, Core_World_Implementation_WorldModule);

