
EZ_FORCE_INLINE bool ezComponentManagerBase::IsValidComponent(const ezComponentHandle& hComponent) const
{
  return m_Components.Contains(hComponent);
}

EZ_FORCE_INLINE bool ezComponentManagerBase::TryGetComponent(const ezComponentHandle& hComponent, ezComponent*& out_pComponent)
{
  return m_Components.TryGetValue(hComponent, out_pComponent);
}

EZ_FORCE_INLINE bool ezComponentManagerBase::TryGetComponent(const ezComponentHandle& hComponent, const ezComponent*& out_pComponent) const
{
  ezComponent* pComponent = nullptr;
  bool res = m_Components.TryGetValue(hComponent, pComponent);
  out_pComponent = pComponent;
  return res;
}

EZ_ALWAYS_INLINE ezUInt32 ezComponentManagerBase::GetComponentCount() const
{
  return static_cast<ezUInt32>(m_Components.GetCount());
}

template <typename ComponentType>
EZ_ALWAYS_INLINE ezTypedComponentHandle<ComponentType> ezComponentManagerBase::CreateComponent(ezGameObject* pOwnerObject, ComponentType*& out_pComponent)
{
  ezComponent* pComponent = nullptr;
  ezComponentHandle hComponent = CreateComponentNoInit(pOwnerObject, pComponent);

  if (pComponent != nullptr)
  {
    InitializeComponent(pComponent);
  }

  out_pComponent = ezStaticCast<ComponentType*>(pComponent);
  return ezTypedComponentHandle<ComponentType>(hComponent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezBlockStorageType::Enum StorageType>
ezComponentManager<T, StorageType>::ezComponentManager(ezWorld* pWorld)
  : ezComponentManagerBase(pWorld)
  , m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  static_assert(EZ_IS_DERIVED_FROM_STATIC(ezComponent, ComponentType), "Not a valid component type");
}

template <typename T, ezBlockStorageType::Enum StorageType>
ezComponentManager<T, StorageType>::~ezComponentManager() = default;

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE bool ezComponentManager<T, StorageType>::TryGetComponent(const ezComponentHandle& hComponent, ComponentType*& out_pComponent)
{
  EZ_ASSERT_DEV(ComponentType::TypeId() == hComponent.GetInternalID().m_TypeId,
    "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
    hComponent.GetInternalID().m_TypeId);
  EZ_ASSERT_DEV(hComponent.GetInternalID().m_WorldIndex == GetWorldIndex(),
    "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), hComponent.GetInternalID().m_WorldIndex);

  ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(hComponent, pComponent);
  out_pComponent = static_cast<ComponentType*>(pComponent);
  return bResult;
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE bool ezComponentManager<T, StorageType>::TryGetComponent(
  const ezComponentHandle& hComponent, const ComponentType*& out_pComponent) const
{
  EZ_ASSERT_DEV(ComponentType::TypeId() == hComponent.GetInternalID().m_TypeId,
    "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
    hComponent.GetInternalID().m_TypeId);
  EZ_ASSERT_DEV(hComponent.GetInternalID().m_WorldIndex == GetWorldIndex(),
    "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), hComponent.GetInternalID().m_WorldIndex);

  const ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(hComponent, pComponent);
  out_pComponent = static_cast<const ComponentType*>(pComponent);
  return bResult;
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator ezComponentManager<T, StorageType>::GetComponents(ezUInt32 uiStartIndex /*= 0*/)
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator
ezComponentManager<T, StorageType>::GetComponents(ezUInt32 uiStartIndex /*= 0*/) const
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

// static
template <typename T, ezBlockStorageType::Enum StorageType>
EZ_ALWAYS_INLINE ezWorldModuleTypeId ezComponentManager<T, StorageType>::TypeId()
{
  return T::TypeId();
}

template <typename T, ezBlockStorageType::Enum StorageType>
void ezComponentManager<T, StorageType>::CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_allComponents, bool bOnlyActive)
{
  out_allComponents.Reserve(out_allComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_allComponents.PushBack(it->GetHandle());
    }
  }
}

template <typename T, ezBlockStorageType::Enum StorageType>
void ezComponentManager<T, StorageType>::CollectAllComponents(ezDynamicArray<ezComponent*>& out_allComponents, bool bOnlyActive)
{
  out_allComponents.Reserve(out_allComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_allComponents.PushBack(it);
    }
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
  if (desc.m_uiAsyncPhaseBatchSize != 0)
  {
    desc.m_uiAsyncPhaseBatchSize = static_cast<ezUInt16>(ezMath::RoundUp(static_cast<ezInt32>(desc.m_uiAsyncPhaseBatchSize), ezDataBlock<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY));
  }

  ezComponentManagerBase::RegisterUpdateFunction(desc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType, ezWorldUpdatePhase::Enum UpdatePhase>
ezComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::ezComponentManagerSimple(ezWorld* pWorld)
  : ezComponentManager<ComponentType, StorageType>(pWorld)
{
}

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType, ezWorldUpdatePhase::Enum UpdatePhase>
void ezComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::Initialize()
{
  using OwnType = ezComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>;

  ezStringBuilder functionName;
  SimpleUpdateName(functionName);

  auto desc = ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&OwnType::SimpleUpdate, this), functionName);
  desc.m_Phase = UpdatePhase;
  desc.m_bOnlyUpdateWhenSimulating = (UpdateType == ezComponentUpdateType::WhenSimulating);

  this->RegisterUpdateFunction(desc);
}

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType, ezWorldUpdatePhase::Enum UpdatePhase>
void ezComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::SimpleUpdate(const ezWorldModule::UpdateContext& context)
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
template <typename ComponentType, ezComponentUpdateType::Enum UpdateType, ezBlockStorageType::Enum StorageType, ezWorldUpdatePhase::Enum UpdatePhase>
void ezComponentManagerSimple<ComponentType, UpdateType, StorageType, UpdatePhase>::SimpleUpdateName(ezStringBuilder& out_sName)
{
  ezStringView sName(EZ_SOURCE_FUNCTION);
  const char* szEnd = sName.FindSubString(",");

  if (szEnd != nullptr && sName.StartsWith("ezComponentManagerSimple<class "))
  {
    ezStringView sChoppedName(sName.GetStartPointer() + ezStringUtils::GetStringElementCount("ezComponentManagerSimple<class "), szEnd);

    EZ_ASSERT_DEV(!sChoppedName.IsEmpty(), "Chopped name is empty: '{0}'", sName);

    out_sName = sChoppedName;
    out_sName.Append("::SimpleUpdate");
  }
  else
  {
    out_sName = sName;
  }
}
