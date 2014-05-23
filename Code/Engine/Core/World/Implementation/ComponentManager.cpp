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

void ezComponentManagerBase::DeleteComponent(const ezComponentHandle& component)
{
  ComponentStorageEntry storageEntry;
  if (m_Components.TryGetValue(component, storageEntry))
  {
    if (ezGameObject* pOwner = storageEntry.m_Ptr->GetOwner())
    {
      pOwner->RemoveComponent(component);
    }

    DeleteComponent(storageEntry);
  }
}

// protected methods

ezComponentHandle ezComponentManagerBase::CreateComponent(ComponentStorageEntry storageEntry, ezUInt16 uiTypeId)
{
  ezGenericComponentId newId = m_Components.Insert(storageEntry);

  ezComponent* pComponent = storageEntry.m_Ptr;
  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;

  return GetHandle(newId, uiTypeId);
}

void ezComponentManagerBase::DeleteComponent(ComponentStorageEntry storageEntry)
{
  ezComponent* pComponent = storageEntry.m_Ptr;
  pComponent->m_InternalId.Invalidate();
  pComponent->m_Flags.Remove(ezObjectFlags::Active);
    
  m_pWorld->m_Data.m_DeadComponents.PushBack(storageEntry);
  m_Components.Remove(pComponent->m_InternalId);
}

void ezComponentManagerBase::DeleteDeadComponent(ComponentStorageEntry storageEntry)
{
  ezGenericComponentId id = storageEntry.m_Ptr->m_InternalId;
  if (id.m_InstanceIndex != ezGenericComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = storageEntry;
}

ezAllocatorBase* ezComponentManagerBase::GetAllocator()
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


EZ_STATICLINK_FILE(Core, Core_World_Implementation_ComponentManager);

