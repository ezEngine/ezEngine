
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

EZ_FORCE_INLINE ezUInt32 ezComponentManagerBase::GetComponentCount() const
{
  return m_Components.GetCount();
}

template <typename ComponentType>
ezComponentHandle ezComponentManagerBase::CreateComponent(ComponentType*& out_pComponent)
{
  ezComponent* pComponent = CreateComponentStorage();
  if (pComponent == nullptr)
  {
    return ezComponentHandle();
  }

  out_pComponent = ezStaticCast<ComponentType*>(pComponent);

  ezGenericComponentId newId = m_Components.Insert(pComponent);

  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;

  ezComponentHandle hComponent = pComponent->GetHandle();
  InitializeComponent(hComponent);

  return hComponent;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, ezBlockStorageType::Enum StorageType>
ezComponentManager<T, StorageType>::ezComponentManager(ezWorld* pWorld) :
  ezComponentManagerBase(pWorld),
  m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponent, ComponentType),
    "Not a valid component type");
}

template <typename T, ezBlockStorageType::Enum StorageType>
ezComponentManager<T, StorageType>::~ezComponentManager()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    DeinitializeComponent(it);
  }
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE bool ezComponentManager<T, StorageType>::TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent)
{
  EZ_ASSERT_DEV(ComponentType::TypeId() == component.GetInternalID().m_TypeId,
    "The given component handle is not of the expected type. Expected type id {0}, got type id {1}",
    ComponentType::TypeId(), component.GetInternalID().m_TypeId);
  EZ_ASSERT_DEV(component.GetInternalID().m_WorldIndex == GetWorld()->GetIndex(),
    "Component does not belong to this world. Expected world id {0} got id {1}", GetWorld()->GetIndex(), component.GetInternalID().m_WorldIndex);

  ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<ComponentType*>(pComponent);
  return bResult;
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE bool ezComponentManager<T, StorageType>::TryGetComponent(const ezComponentHandle& component, const ComponentType*& out_pComponent) const
{
  EZ_ASSERT_DEV(ComponentType::TypeId() == component.GetInternalID().m_TypeId,
                "The given component handle is not of the expected type. Expected type id {0}, got type id {1}",
    ComponentType::TypeId(), component.GetInternalID().m_TypeId);
  EZ_ASSERT_DEV(component.GetInternalID().m_WorldIndex == GetWorld()->GetIndex(),
                "Component does not belong to this world. Expected world id {0} got id {1}", GetWorld()->GetIndex(), component.GetInternalID().m_WorldIndex);

  const ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<const ComponentType*>(pComponent);
  return bResult;
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator ezComponentManager<T, StorageType>::GetComponents()
{
  return m_ComponentStorage.GetIterator();
}

template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator ezComponentManager<T, StorageType>::GetComponents() const
{
  return m_ComponentStorage.GetIterator();
}

//static
template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE ezUInt16 ezComponentManager<T, StorageType>::TypeId()
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
EZ_FORCE_INLINE ezComponent* ezComponentManager<T, StorageType>::CreateComponentStorage()
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
    desc.m_uiGranularity = ezMath::Ceil((ezInt32)desc.m_uiGranularity, ezDataBlock<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY);

  ezComponentManagerBase::RegisterUpdateFunction(desc);
}

//static
template <typename T, ezBlockStorageType::Enum StorageType>
EZ_FORCE_INLINE ezUInt16 ezComponentManager<T, StorageType>::GetNextTypeId()
{
  return ezWorldModule::GetNextTypeId();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType>
ezComponentManagerSimple<ComponentType, UpdateType>::ezComponentManagerSimple(ezWorld* pWorld)
  : ezComponentManager<ComponentType, ezBlockStorageType::FreeList>(pWorld)
{
}

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType>
void ezComponentManagerSimple<ComponentType, UpdateType>::Initialize()
{
  typedef ezComponentManagerSimple<ComponentType, UpdateType> OwnType;

  ezStringBuilder functionName;
  SimpleUpdateName(functionName);

  auto desc = ezWorldModule::UpdateFunctionDesc(ezWorldModule::UpdateFunction(&OwnType::SimpleUpdate, this), functionName);
  desc.m_bOnlyUpdateWhenSimulating = (UpdateType == ezComponentUpdateType::WhenSimulating);

  this->RegisterUpdateFunction(desc);
}

template <typename ComponentType, ezComponentUpdateType::Enum UpdateType>
void ezComponentManagerSimple<ComponentType, UpdateType>::SimpleUpdate(const ezWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      //EZ_ASSERT_DEV(!this->GetWorldSimulationEnabled() || pComponent->IsSimulationStarted(), "Implementation error: simulation start must be called before any update method.");

      if (this->GetWorldSimulationEnabled() && !pComponent->IsSimulationStarted())
        ezLog::Warning("Bad component initialization?");

      pComponent->Update();
    }
  }
}

//static
template <typename ComponentType, ezComponentUpdateType::Enum UpdateType>
void ezComponentManagerSimple<ComponentType, UpdateType>::SimpleUpdateName(ezStringBuilder& out_sName)
{
  ezStringView sName(EZ_SOURCE_FUNCTION);
  const char* szEnd = sName.FindSubString(",");
  ezStringView sChoppedName(sName.GetData() + ::strlen("ezComponentManagerSimple<class "), szEnd);

  out_sName = sChoppedName;
  out_sName.Append("::SimpleUpdate");
}

//////////////////////////////////////////////////////////////////////////

template <typename ComponentType>
ezUInt16 ezComponentManagerFactory::RegisterComponentManager()
{
  const ezRTTI* pRtti = ezGetStaticRTTI<ComponentType>();
  ezUInt16 uiTypeId = -1;
  if (m_TypeToId.TryGetValue(pRtti, uiTypeId))
  {
    return uiTypeId;
  }

  uiTypeId = ComponentType::ComponentManagerType::GetNextTypeId();
  m_TypeToId.Insert(pRtti, uiTypeId);

  struct Helper
  {
    static ezComponentManagerBase* Create(ezAllocatorBase* pAllocator, ezWorld* pWorld)
    {
      return EZ_NEW(pAllocator, typename ComponentType::ComponentManagerType, pWorld);
    }
  };

  if (uiTypeId >= m_CreatorFuncs.GetCount())
  {
    m_CreatorFuncs.SetCount(uiTypeId + 1);
  }

  m_CreatorFuncs[uiTypeId] = &Helper::Create;

  return uiTypeId;
}
