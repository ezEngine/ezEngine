#include <Core/PCH.h>
#include <Core/World/World.h>

ezComponentManagerBase::ezComponentManagerBase(ezWorld* pWorld)
  : ezWorldModule(pWorld)
  , m_Components(pWorld->GetAllocator())
{
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

ezComponentHandle ezComponentManagerBase::CreateComponentEntry(ComponentStorageEntry storageEntry)
{
  ezGenericComponentId newId = m_Components.Insert(storageEntry);

  ezComponent* pComponent = storageEntry.m_Ptr;
  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;

  ezComponentHandle hComponent = pComponent->GetHandle();
  GetWorld()->AddComponentToInitialize(hComponent);

  return hComponent;
}

void ezComponentManagerBase::DeinitializeComponent(ezComponent* pComponent)
{
  if (ezGameObject* pOwner = pComponent->GetOwner())
  {
    pOwner->DetachComponent(pComponent);
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

  GetWorld()->m_Data.m_DeadComponents.PushBack(storageEntry);
}

void ezComponentManagerBase::DeleteDeadComponent(ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent)
{
  ezGenericComponentId id = storageEntry.m_Ptr->m_InternalId;
  if (id.m_InstanceIndex != ezGenericComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = storageEntry;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

ezComponentManagerFactory::ezComponentManagerFactory()
{
}

ezComponentManagerFactory* ezComponentManagerFactory::GetInstance()
{
  static ezComponentManagerFactory* pInstance = new ezComponentManagerFactory();
  return pInstance;
}

ezUInt16 ezComponentManagerFactory::GetTypeId(const ezRTTI* pRtti)
{
  ezUInt16 uiTypeId = 0xFFFF;
  m_TypeToId.TryGetValue(pRtti, uiTypeId);
  return uiTypeId;
}

ezComponentManagerBase* ezComponentManagerFactory::CreateComponentManager(ezUInt16 typeId, ezWorld* pWorld)
{
  if (typeId < m_CreatorFuncs.GetCount())
  {
    CreatorFunc func = m_CreatorFuncs[typeId];
    return (*func)(pWorld->GetAllocator(), pWorld);
  }

  return nullptr;
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_ComponentManager);

