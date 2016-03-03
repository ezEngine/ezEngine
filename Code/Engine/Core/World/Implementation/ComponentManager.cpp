#include <Core/PCH.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/World.h>

ezComponentManagerBase::ezComponentManagerBase(ezWorld* pWorld)
  : m_Components(pWorld->GetAllocator())
{
  m_pWorld = pWorld;
  m_pUserData = nullptr;
}

ezComponentManagerBase::~ezComponentManagerBase()
{
}

void ezComponentManagerBase::DeleteComponent(const ezComponentHandle& component)
{
  ComponentStorageEntry storageEntry;
  if (m_Components.TryGetValue(component, storageEntry))
  {
    DeleteComponentEntry(storageEntry);
  }
}

// protected methods

void ezComponentManagerBase::RegisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->RegisterUpdateFunction(desc);
}

void ezComponentManagerBase::DeregisterUpdateFunction(const UpdateFunctionDesc& desc)
{
  m_pWorld->DeregisterUpdateFunction(desc);
}

ezComponentHandle ezComponentManagerBase::CreateComponentEntry(ComponentStorageEntry storageEntry, ezUInt16 uiTypeId)
{
  ezGenericComponentId newId = m_Components.Insert(storageEntry);

  ezComponent* pComponent = storageEntry.m_Ptr;
  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;

  GetWorld()->AddComponentToInitialize(pComponent->GetHandle());

  return GetHandle(newId, uiTypeId);
}

void ezComponentManagerBase::DeinitializeComponent(ezComponent* pComponent)
{
  if (ezGameObject* pOwner = pComponent->GetOwner())
  {
    pOwner->RemoveComponent(pComponent);
  }

  if (pComponent->IsInitialized())
  {
    pComponent->Deinitialize();
    pComponent->m_ComponentFlags.Remove(ezObjectFlags::Initialized);
  }
}

void ezComponentManagerBase::DeleteComponentEntry(ComponentStorageEntry storageEntry)
{
  ezComponent* pComponent = storageEntry.m_Ptr;
  DeinitializeComponent(pComponent);

  m_Components.Remove(pComponent->m_InternalId);

  pComponent->m_InternalId.Invalidate();
  pComponent->m_ComponentFlags.Remove(ezObjectFlags::Active);
    
  m_pWorld->m_Data.m_DeadComponents.PushBack(storageEntry);  
}

void ezComponentManagerBase::DeleteDeadComponent(ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent)
{
  ezGenericComponentId id = storageEntry.m_Ptr->m_InternalId;
  if (id.m_InstanceIndex != ezGenericComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = storageEntry;
}

ezAllocatorBase* ezComponentManagerBase::GetAllocator()
{
  return m_pWorld->GetAllocator();
}

ezInternal::WorldLargeBlockAllocator* ezComponentManagerBase::GetBlockAllocator()
{
  return m_pWorld->GetBlockAllocator();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

ezUInt16 ezComponentManagerFactory::s_uiNextTypeId = 0;
ezHashTable<const ezRTTI*, ezUInt16> ezComponentManagerFactory::s_TypeToId;
ezDynamicArray<ezComponentManagerFactory::CreatorFunc> ezComponentManagerFactory::s_CreatorFuncs;

// static 
ezUInt16 ezComponentManagerFactory::GetTypeId(const ezRTTI* pRtti)
{
  ezUInt16 uiTypeId = -1;
  s_TypeToId.TryGetValue(pRtti, uiTypeId);
  return uiTypeId;
}

// static 
ezComponentManagerBase* ezComponentManagerFactory::CreateComponentManager(ezUInt16 typeId, ezWorld* pWorld)
{
  if (typeId < s_CreatorFuncs.GetCount())
  {
    CreatorFunc func = s_CreatorFuncs[typeId];
    return (*func)(pWorld->GetAllocator(), pWorld);
  }

  return nullptr;
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_ComponentManager);

