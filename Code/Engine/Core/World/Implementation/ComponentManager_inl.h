
EZ_FORCE_INLINE bool ezComponentManagerBase::IsValidComponent(const ezComponentHandle& component) const
{
  return m_Components.Contains(component);
}

EZ_FORCE_INLINE bool ezComponentManagerBase::TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent)
{
  return m_Components.TryGetValue(component, out_pComponent);
}

EZ_FORCE_INLINE bool ezComponentManagerBase::TryGetComponent(const ezComponentHandle& component, const ezComponent*& out_pComponent) const
{
  ezComponent* pComponent = nullptr;
  bool res = m_Components.TryGetValue(component, pComponent);
  out_pComponent = pComponent;
  return res;
}

EZ_ALWAYS_INLINE ezUInt32 ezComponentManagerBase::GetComponentCount() const
{
  return m_Components.GetCount();
}

template <typename ComponentType>
ezComponentHandle ezComponentManagerBase::CreateComponent(ezGameObject* pOwnerObject, ComponentType*& out_pComponent)
{
  ezComponent* pComponent = CreateComponentStorage();
  if (pComponent == nullptr)
  {
    return ezComponentHandle();
  }

  ezGenericComponentId newId = m_Components.Insert(pComponent);

  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;
  pComponent->m_ComponentFlags.AddOrRemove(ezObjectFlags::Dynamic, pComponent->GetMode() == ezComponentMode::Dynamic);

  InitializeComponent(pOwnerObject, pComponent);

  out_pComponent = ezStaticCast<ComponentType*>(pComponent);
  return pComponent->GetHandle();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezBlockStorageType::Enum StorageType>
ezComponentManager<T, StorageType>::ezComponentManager(ezWorld* pWorld)
    : ezComponentManagerBase(pWorld)
    , m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponent, ComponentType), "Not a valid component type");
}

template <typename T, ezBlockStorageType::Enum StorageType>
ezComponentManager<T, StorageType>::~ezComponentManager()
{
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE bool ezComponentManager<T, StorageType>::TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent)
{
  EZ_ASSERT_DEV(ComponentType::TypeId() == component.GetInternalID().m_TypeId,
                "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
                component.GetInternalID().m_TypeId);
  EZ_ASSERT_DEV(component.GetInternalID().m_WorldIndex == GetWorld()->GetIndex(),
                "Component does not belong to this world. Expected world id {0} got id {1}", GetWorld()->GetIndex(),
                component.GetInternalID().m_WorldIndex);

  ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<ComponentType*>(pComponent);
  return bResult;
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE bool ezComponentManager<T, StorageType>::TryGetComponent(const ezComponentHandle& component,
                                                                         const ComponentType*& out_pComponent) const
{
  EZ_ASSERT_DEV(ComponentType::TypeId() == component.GetInternalID().m_TypeId,
                "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
                component.GetInternalID().m_TypeId);
  EZ_ASSERT_DEV(component.GetInternalID().m_WorldIndex == GetWorld()->GetIndex(),
                "Component does not belong to this world. Expected world id {0} got id {1}", GetWorld()->GetIndex(),
                component.GetInternalID().m_WorldIndex);

  const ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<const ComponentType*>(pComponent);
  return bResult;
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator
ezComponentManager<T, StorageType>::GetComponents()
{
  return m_ComponentStorage.GetIterator();
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator
ezComponentManager<T, StorageType>::GetComponents() const
{
  return m_ComponentStorage.GetIterator();
}

// static
template <typename T, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE ezUInt16 ezComponentManager<T, StorageType>::TypeId()
{
  return T::TypeId();
}

template <typename T, ezBlockStorageType::Enum StorageType>
void ezComponentManager<T, StorageType>::CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_AllComponents)
{
  out_AllComponents.Reserve(out_AllComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    out_AllComponents.PushBack(it->GetHandle());
  }
}

template <typename T, ezBlockStorageType::Enum StorageType>
void ezComponentManager<T, StorageType>::CollectAllComponents(ezDynamicArray<ezComponent*>& out_AllComponents)
{
  out_AllComponents.Reserve(out_AllComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    out_AllComponents.PushBack(it);
  }
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE ezComponent* ezComponentManager<T, StorageType>::CreateComponentStorage()
{
  return m_ComponentStorage.Create();
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE void ezComponentManager<T, StorageType>::DeleteComponentStorage(ezComponent* pComponent, ezComponent*& out_pMovedComponent)
{
  T* pMovedComponent = nullptr;
  m_ComponentStorage.Delete(static_cast<T*>(pComponent), pMovedComponent);
  out_pMovedComponent = pMovedComponent;
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE void ezComponentManager<T, StorageType>::RegisterUpdateFunction(UpdateFunctionDesc& desc)
{
  // round up to multiple of data block capacity so tasks only have to deal with complete data blocks
  if (desc.m_uiGranularity != 0)
    desc.m_uiGranularity =
        ezMath::RoundUp((ezInt32)desc.m_uiGranularity, ezDataBlock<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY);

  ezComponentManagerBase::RegisterUpdateFunction(desc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType>
ezComponentManagerSimple<ComponentType, UpdateType, StorageType>::ezComponentManagerSimple(ezWorld* pWorld)
    : ezComponentManager<ComponentType, StorageType>(pWorld)
{
}

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType>
void ezComponentManagerSimple<ComponentType, UpdateType, StorageType>::Initialize()
{
  typedef ezComponentManagerSimple<ComponentType, UpdateType> OwnType;

  ezStringBuilder functionName;
  SimpleUpdateName(functionName);

  auto desc = ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&OwnType::SimpleUpdate, this), functionName);
  desc.m_bOnlyUpdateWhenSimulating = (UpdateType == ezComponentUpdateType::WhenSimulating);

  this->RegisterUpdateFunction(desc);
}

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType>
void ezComponentManagerSimple<ComponentType, UpdateType, StorageType>::SimpleUpdate(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// static
template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType>
void ezComponentManagerSimple<ComponentType, UpdateType, StorageType>::SimpleUpdateName(ezStringBuilder& out_sName)
{
  ezStringView sName(EZ_SOURCE_FUNCTION);
  const char* szEnd = sName.FindSubString(",");

  if (szEnd != nullptr && sName.StartsWith("ezComponentManagerSimple<class "))
  {
    ezStringView sChoppedName(sName.GetData() + ezStringUtils::GetStringElementCount("ezComponentManagerSimple<class "), szEnd);

    EZ_ASSERT_DEV(!sChoppedName.IsEmpty(), "Chopped name is empty: '{0}'", sName);

    out_sName = sChoppedName;
    out_sName.Append("::SimpleUpdate");
  }
  else
  {
    out_sName = sName;
  }
}
