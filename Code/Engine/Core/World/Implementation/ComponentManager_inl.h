
EZ_FORCE_INLINE ezWorld* ezComponentManagerBase::GetWorld() const
{
  return m_pWorld;
}

EZ_FORCE_INLINE ezComponent* ezComponentManagerBase::GetComponent(const ezComponentHandle& component) const
{
  ezComponent* pComponent = NULL;
  m_Components.TryGetValue(component.m_InternalId, pComponent);
  return pComponent;
}

EZ_FORCE_INLINE ezUInt32 ezComponentManagerBase::GetComponentCount() const
{
  return m_Components.GetCount();
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
}

template <typename ComponentType>
ezComponentManager<ComponentType>::~ezComponentManager()
{
}

template <typename ComponentType>
EZ_FORCE_INLINE ezComponentHandle ezComponentManager<ComponentType>::CreateComponent()
{
  ComponentType* pNewComponent = m_ComponentStorage.Create();
  ezMemoryUtils::Construct(pNewComponent, 1);

  return ezComponentManagerBase::CreateComponent(pNewComponent, ComponentType::TypeId());
}

template <typename ComponentType>
void ezComponentManager<ComponentType>::DeleteComponent(const ezComponentHandle& component)
{
}

template <typename ComponentType>
EZ_FORCE_INLINE ComponentType* ezComponentManager<ComponentType>::GetComponent(const ezComponentHandle& component) const
{
  EZ_ASSERT(ComponentType::TypeId() == GetIdFromHandle(component).m_TypeId, 
    "The given component handle is not of the expected type. Expected type id %d, got type id %d",
    ComponentType::TypeId(), GetIdFromHandle(component).m_TypeId);

  return static_cast<ComponentType*>(ezComponentManagerBase::GetComponent(component));
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
  struct UpdateFunctor
  {
    EZ_FORCE_INLINE void operator()(ComponentType& component)
    {
      component.Update();
    }
  };

  UpdateFunctor functor;
  m_ComponentStorage.Iterate(functor, uiStartIndex, uiCount);
}
