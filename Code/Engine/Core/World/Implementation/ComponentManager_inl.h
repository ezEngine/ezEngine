
EZ_FORCE_INLINE ezWorld* ezComponentManagerBase::GetWorld() const
{
  return m_pWorld;
}

EZ_FORCE_INLINE bool ezComponentManagerBase::IsValidComponent(const ezComponentHandle& component) const
{
  return m_Components.Contains(component.m_InternalId);
}

EZ_FORCE_INLINE bool ezComponentManagerBase::TryGetComponent(const ezComponentHandle& component, ezComponent*& out_pComponent) const
{
  ComponentStorageEntry storageEntry = { NULL };
  bool res = m_Components.TryGetValue(component.m_InternalId, storageEntry);
  out_pComponent = storageEntry.m_Ptr;
  return res;
}

EZ_FORCE_INLINE ezUInt32 ezComponentManagerBase::GetComponentCount() const
{
  return m_Components.GetCount();
}

EZ_FORCE_INLINE ezUInt32 ezComponentManagerBase::GetActiveComponentCount() const
{
  return m_uiActiveComponentCount;
}

//static 
EZ_FORCE_INLINE ezUInt16 ezComponentManagerBase::GetNextTypeId()
{
  return s_uiNextTypeId++;
}

//static
EZ_FORCE_INLINE ezComponentId ezComponentManagerBase::GetIdFromHandle(const ezComponentHandle& component)
{
  return component.m_InternalId;
}

//static
EZ_FORCE_INLINE ezComponentHandle ezComponentManagerBase::GetHandle(ezGenericComponentId internalId, ezUInt16 uiTypeId)
{
  return ezComponentHandle(ezComponentId(internalId, uiTypeId));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType>
ezComponentManager<ComponentType>::ezComponentManager(ezWorld* pWorld) : 
  ezComponentManagerBase(pWorld),
  m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponent, ComponentType),
    "Not a valid component type");
}

template <typename ComponentType>
ezComponentManager<ComponentType>::~ezComponentManager()
{
}

template <typename ComponentType>
EZ_FORCE_INLINE ezComponentHandle ezComponentManager<ComponentType>::CreateComponent()
{
  ComponentType* pNewComponent = NULL;
  return CreateComponent(pNewComponent);
}

template <typename ComponentType>
EZ_FORCE_INLINE ezComponentHandle ezComponentManager<ComponentType>::CreateComponent(ComponentType*& out_pComponent)
{
  ezBlockStorage<ComponentType>::Entry storageEntry = m_ComponentStorage.Create();
  out_pComponent = storageEntry.m_Ptr;

  return ezComponentManagerBase::CreateComponent(*reinterpret_cast<ComponentStorageEntry*>(&storageEntry), ComponentType::TypeId());
}

template <typename ComponentType>
EZ_FORCE_INLINE bool ezComponentManager<ComponentType>::TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent) const
{
  EZ_ASSERT(ComponentType::TypeId() == GetIdFromHandle(component).m_TypeId, 
    "The given component handle is not of the expected type. Expected type id %d, got type id %d",
    ComponentType::TypeId(), GetIdFromHandle(component).m_TypeId);

  return ezComponentManagerBase::TryGetComponent(component, out_pComponent);
}

template <typename ComponentType>
EZ_FORCE_INLINE typename ezBlockStorage<ComponentType>::Iterator ezComponentManager<ComponentType>::GetComponents()
{
  return m_ComponentStorage.GetIterator();
}

template <typename ComponentType>
EZ_FORCE_INLINE void ezComponentManager<ComponentType>::DeleteDeadComponent(ComponentStorageEntry storageEntry)
{
  m_ComponentStorage.Delete(*reinterpret_cast<ezBlockStorage<ComponentType>::Entry*>(&storageEntry));
  ezComponentManagerBase::DeleteDeadComponent(storageEntry);
}

template <typename ComponentType>
EZ_FORCE_INLINE void ezComponentManager<ComponentType>::RegisterUpdateFunction(UpdateFunctionDesc& desc)
{
  // round up to multiple of data block capacity so tasks only have to deal with complete data blocks
  if (desc.m_uiGranularity != 0)
    desc.m_uiGranularity = ezMath::Ceil((ezInt32)desc.m_uiGranularity, ezDataBlock<ComponentType>::CAPACITY);

  ezComponentManagerBase::RegisterUpdateFunction(desc);
}

//static
template <typename ComponentType>
EZ_FORCE_INLINE ezComponentHandle ezComponentManager<ComponentType>::GetHandle(ezGenericComponentId internalId)
{
  return ezComponentManagerBase::GetHandle(internalId, ComponentType::TypeId());
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType>
ezComponentManagerSimple<ComponentType>::ezComponentManagerSimple(ezWorld* pWorld) : 
  ezComponentManager(pWorld)
{
}

template <typename ComponentType>
ezResult ezComponentManagerSimple<ComponentType>::Initialize()
{
  UpdateFunctionDesc desc;
  desc.m_Function = UpdateFunction(&ezComponentManagerSimple<ComponentType>::SimpleUpdate, this);

  RegisterUpdateFunction(desc);

  return EZ_SUCCESS;
}

template <typename ComponentType>
void ezComponentManagerSimple<ComponentType>::SimpleUpdate(ezUInt32 uiStartIndex, ezUInt32 uiCount)
{
  for (ezBlockStorage<ComponentType>::Iterator it = m_ComponentStorage.GetIterator(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    ComponentType& component = *it;
    if (component.IsActive())
    {
      component.Update();
    }
  }
}
