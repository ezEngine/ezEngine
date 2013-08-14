#include <Core/PCH.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>

ezUInt16 ezComponentManagerBase::s_uiNextTypeId;

ezComponentManagerBase::ezComponentManagerBase(ezWorld* pWorld) : 
  m_Components(pWorld->GetAllocator())
{
  m_uiActiveComponentCount = 0;
  m_pWorld = pWorld;
}

ezComponentManagerBase::~ezComponentManagerBase()
{
}

void ezComponentManagerBase::DeleteComponent(const ezComponentHandle& component)
{
  ComponentStorageEntry storageEntry;
  if (m_Components.TryGetValue(component.m_InternalId, storageEntry))
  {
    ezComponent* pComponent = storageEntry.m_Ptr;
    pComponent->m_InternalId = ezGenericComponentId();
    pComponent->m_Flags.Remove(ezObjectFlags::Active);
    m_pWorld->m_Data.m_DeadComponents.PushBack(storageEntry);
    m_Components.Remove(component.m_InternalId);
  }
}

ezComponentHandle ezComponentManagerBase::CreateComponent(ComponentStorageEntry storageEntry, ezUInt16 uiTypeId)
{
  ezGenericComponentId newId = m_Components.Insert(storageEntry);

  ezComponent* pComponent = storageEntry.m_Ptr;
  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;

  if (pComponent->IsActive())
    ++m_uiActiveComponentCount;
  
  return GetHandle(newId, uiTypeId);
}

void ezComponentManagerBase::DeleteDeadComponent(ComponentStorageEntry storageEntry)
{
  ezGenericComponentId id = storageEntry.m_Ptr->m_InternalId;
  if (id.m_InstanceIndex != ezGenericComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = storageEntry;

  --m_uiActiveComponentCount;
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
