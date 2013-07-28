#include <Core/PCH.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>

ezUInt16 ezComponentManagerBase::s_uiNextTypeId;

ezComponentManagerBase::ezComponentManagerBase(ezWorld* pWorld) : 
  m_Components(pWorld->GetAllocator())
{
  m_pWorld = pWorld;
}

ezComponentManagerBase::~ezComponentManagerBase()
{
}

ezComponentHandle ezComponentManagerBase::CreateComponent(ezComponent* pComponent, ezUInt16 uiTypeId)
{
  ezGenericComponentId newId = m_Components.Insert(pComponent);

  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;
  
  return GetHandle(newId, uiTypeId);
}

ezIAllocator* ezComponentManagerBase::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

ezLargeBlockAllocator* ezComponentManagerBase::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

void ezComponentManagerBase::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void ezComponentManagerBase::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}
