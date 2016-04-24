
EZ_FORCE_INLINE ezWorld* ezComponentManagerBase::GetWorld()
{
  return m_pWorld;
}

EZ_FORCE_INLINE const ezWorld* ezComponentManagerBase::GetWorld() const
{
  return m_pWorld;
}

EZ_FORCE_INLINE bool ezComponentManagerBase::IsValidComponent(const ezComponentHandle& component) const
{
  return m_Components.Contains(component);
}

EZ_FORCE_INLINE bool ezComponentManagerBase::TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent)
{
  ComponentStorageEntry storageEntry = { nullptr };
  bool res = m_Components.TryGetValue(component, storageEntry);
  out_pComponent = storageEntry.m_Ptr;
  return res;
}

EZ_FORCE_INLINE bool ezComponentManagerBase::TryGetComponent(const ezComponentHandle& component, const ezComponent*& out_pComponent) const
{
  ComponentStorageEntry storageEntry = { nullptr };
  bool res = m_Components.TryGetValue(component, storageEntry);
  out_pComponent = storageEntry.m_Ptr;
  return res;
}

EZ_FORCE_INLINE ezUInt32 ezComponentManagerBase::GetComponentCount() const
{
  return m_Components.GetCount();
}

//static
EZ_FORCE_INLINE ezComponentId ezComponentManagerBase::GetIdFromHandle(const ezComponentHandle& component)
{
  return component;
}

//static
EZ_FORCE_INLINE ezComponentHandle ezComponentManagerBase::GetHandle(ezGenericComponentId internalId, ezUInt16 uiTypeId)
{
  return ezComponentHandle(ezComponentId(internalId, uiTypeId));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, bool CompactStorage>
ezComponentManager<T, CompactStorage>::ezComponentManager(ezWorld* pWorld) : 
  ezComponentManagerBase(pWorld),
  m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponent, ComponentType),
    "Not a valid component type");
}

template <typename T, bool CompactStorage>
ezComponentManager<T, CompactStorage>::~ezComponentManager()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    DeinitializeComponent(it);
  }
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE ezComponentHandle ezComponentManager<T, CompactStorage>::AllocateComponent()
{
  ComponentType* pNewComponent = nullptr;
  return CreateComponent(pNewComponent);
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE ezComponentHandle ezComponentManager<T, CompactStorage>::CreateComponent(ComponentType*& out_pComponent)
{
  auto storageEntry = m_ComponentStorage.Create();
  out_pComponent = storageEntry.m_Ptr;

  return ezComponentManagerBase::CreateComponentEntry(*reinterpret_cast<ComponentStorageEntry*>(&storageEntry), ComponentType::TypeId());
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE bool ezComponentManager<T, CompactStorage>::TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent)
{
  EZ_ASSERT_DEBUG(ComponentType::TypeId() == GetIdFromHandle(component).m_TypeId, 
    "The given component handle is not of the expected type. Expected type id %d, got type id %d",
    ComponentType::TypeId(), GetIdFromHandle(component).m_TypeId);

  ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<ComponentType*>(pComponent);
  return bResult;
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE bool ezComponentManager<T, CompactStorage>::TryGetComponent(const ezComponentHandle& component, const ComponentType*& out_pComponent) const
{
  EZ_ASSERT_DEBUG(ComponentType::TypeId() == GetIdFromHandle(component).m_TypeId,
    "The given component handle is not of the expected type. Expected type id %d, got type id %d",
    ComponentType::TypeId(), GetIdFromHandle(component).m_TypeId);

  const ezComponent* pComponent = nullptr;
  bool bResult = ezComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<const ComponentType*>(pComponent);
  return bResult;
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, CompactStorage>::Iterator ezComponentManager<T, CompactStorage>::GetComponents()
{
  return m_ComponentStorage.GetIterator();
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE typename ezBlockStorage<T, ezInternal::DEFAULT_BLOCK_SIZE, CompactStorage>::ConstIterator ezComponentManager<T, CompactStorage>::GetComponents() const
{
  return m_ComponentStorage.GetIterator();
}

template <typename T, bool CompactStorage>
const ezRTTI* ezComponentManager<T, CompactStorage>::GetComponentType() const
{
  return ezGetStaticRTTI<T>();
}

//static
template <typename T, bool CompactStorage>
ezUInt16 ezComponentManager<T, CompactStorage>::TypeId()
{
  return T::TypeId();
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE void ezComponentManager<T, CompactStorage>::DeleteDeadComponent(ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent)
{
  T* pMovedComponent = nullptr;
  m_ComponentStorage.Delete(*reinterpret_cast<typename ezBlockStorage<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE, CompactStorage>::Entry*>(&storageEntry), pMovedComponent);
  out_pMovedComponent = pMovedComponent;

  ezComponentManagerBase::DeleteDeadComponent(storageEntry, out_pMovedComponent);
}

template <typename T, bool CompactStorage>
EZ_FORCE_INLINE void ezComponentManager<T, CompactStorage>::RegisterUpdateFunction(UpdateFunctionDesc& desc)
{
  // round up to multiple of data block capacity so tasks only have to deal with complete data blocks
  if (desc.m_uiGranularity != 0)
    desc.m_uiGranularity = ezMath::Ceil((ezInt32)desc.m_uiGranularity, ezDataBlock<ComponentType, ezInternal::DEFAULT_BLOCK_SIZE>::CAPACITY);

  ezComponentManagerBase::RegisterUpdateFunction(desc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType, bool OnlyUpdateWhenSimulating>
ezComponentManagerSimple<ComponentType, OnlyUpdateWhenSimulating>::ezComponentManagerSimple(ezWorld* pWorld) :
  ezComponentManager<ComponentType>(pWorld)
{
}

template <typename ComponentType, bool OnlyUpdateWhenSimulating>
void ezComponentManagerSimple<ComponentType, OnlyUpdateWhenSimulating>::Initialize()
{
  typedef ezComponentManagerSimple<ComponentType, OnlyUpdateWhenSimulating> OwnType;

  auto desc = EZ_CREATE_COMPONENT_UPDATE_FUNCTION_DESC(OwnType::SimpleUpdate, this);

  this->RegisterUpdateFunction(desc);
}

template <typename ComponentType, bool OnlyUpdateWhenSimulating>
void ezComponentManagerSimple<ComponentType, OnlyUpdateWhenSimulating>::SimpleUpdate(ezUInt32 uiStartIndex, ezUInt32 uiCount)
{
  // ignore the update when this is a 'simulating' component type and the world is not in simulation mode
  if (OnlyUpdateWhenSimulating && !ezComponentManagerBase::GetWorld()->GetWorldSimulationEnabled())
    return;

  for (auto it = this->m_ComponentStorage.GetIterator(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
      it->Update();
  }
}

//////////////////////////////////////////////////////////////////////////


template <typename ComponentType>
ezComponentHandle ezSettingsComponentManager<ComponentType>::AllocateComponent()
{
  ezComponentHandle hComp = ezComponentManager<ComponentType>::AllocateComponent();

  ComponentType* pComponent = nullptr;
  TryGetComponent(hComp, pComponent);

  if (!m_pSingleton)
  {
    m_pSingleton = pComponent;
  }
  else
  {
    ezLog::Error("A component of type '%s' is already present in this world. Having more than one may lead to unexpected behavior.", ezGetStaticRTTI<ComponentType>()->GetTypeName());
  }

  return hComp;
}


template <typename ComponentType>
void ezSettingsComponentManager<ComponentType>::DeleteDeadComponent(ezComponentManagerBase::ComponentStorageEntry storageEntry, ezComponent*& out_pMovedComponent)
{
  if (out_pMovedComponent == m_pSingleton)
  {
    m_pSingleton = nullptr;
  }

  ezComponentManager<ComponentType>::DeleteDeadComponent(storageEntry, out_pMovedComponent);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType>
ezUInt16 ezComponentManagerFactory::RegisterComponentManager()
{
  const ezRTTI* pRtti = ezGetStaticRTTI<ComponentType>();
  ezUInt16 uiTypeId = -1;
  if (s_TypeToId.TryGetValue(pRtti, uiTypeId))
  {
    return uiTypeId;
  }

  uiTypeId = s_uiNextTypeId++;
  s_TypeToId.Insert(pRtti, uiTypeId);

  struct Helper
  {
    static ezComponentManagerBase* Create(ezAllocatorBase* pAllocator, ezWorld* pWorld)
    {
      return EZ_NEW(pAllocator, typename ComponentType::ComponentManagerType, pWorld);
    }
  };

  EZ_ASSERT_DEV(s_CreatorFuncs.GetCount() == uiTypeId, "");
  s_CreatorFuncs.PushBack(&Helper::Create);

  return uiTypeId;
}