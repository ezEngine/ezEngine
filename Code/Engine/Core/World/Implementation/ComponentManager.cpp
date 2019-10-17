#include <CorePCH.h>

#include <Core/World/World.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezComponentManagerBase, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezComponentManagerBase::ezComponentManagerBase(ezWorld* pWorld)
    : ezWorldModule(pWorld)
    , m_Components(pWorld->GetAllocator())
{
}

ezComponentManagerBase::~ezComponentManagerBase() {}

ezComponentHandle ezComponentManagerBase::CreateComponent(ezGameObject* pOwnerObject)
{
  ezComponent* pDummy;
  return CreateComponent(pOwnerObject, pDummy);
}

void ezComponentManagerBase::DeleteComponent(const ezComponentHandle& component)
{
  ezComponent* pComponent = nullptr;
  if (!m_Components.TryGetValue(component, pComponent))
    return;

  DeleteComponent(pComponent);
}

void ezComponentManagerBase::DeleteComponent(ezComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  DeinitializeComponent(pComponent);

  m_Components.Remove(pComponent->m_InternalId);

  pComponent->m_InternalId.Invalidate();
  pComponent->m_ComponentFlags.Remove(ezObjectFlags::Active);

  GetWorld()->m_Data.m_DeadComponents.Insert(pComponent);
}

void ezComponentManagerBase::DeinitializeInternal()
{
  for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
  {
    DeinitializeComponent(it.Value());
  }

  SUPER::DeinitializeInternal();
}

void ezComponentManagerBase::InitializeComponent(ezGameObject* pOwnerObject, ezComponent* pComponent)
{
  // In Editor we add components via reflection so it is fine to have a nullptr here.
  // We check for a valid owner before the Initialize() callback.
  if (pOwnerObject != nullptr)
  {
    pOwnerObject->AddComponent(pComponent);
  }

  GetWorld()->AddComponentToInitialize(pComponent->GetHandle());
}

void ezComponentManagerBase::DeinitializeComponent(ezComponent* pComponent)
{
  if (pComponent->IsInitialized())
  {
    pComponent->Deinitialize();
    pComponent->m_ComponentFlags.Remove(ezObjectFlags::Initialized);
  }

  if (ezGameObject* pOwner = pComponent->GetOwner())
  {
    pOwner->RemoveComponent(pComponent);
  }
}

void ezComponentManagerBase::PatchIdTable(ezComponent* pComponent)
{
  ezGenericComponentId id = pComponent->m_InternalId;
  if (id.m_InstanceIndex != ezGenericComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = pComponent;
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_ComponentManager);

