
template <typename ComponentType>
ezSettingsComponentManager<ComponentType>::ezSettingsComponentManager(ezWorld* pWorld)
  : ezComponentManagerBase(pWorld)
{
}

template <typename ComponentType>
ezSettingsComponentManager<ComponentType>::~ezSettingsComponentManager()
{
  if (m_pSingletonComponent != nullptr)
  {
    DeinitializeComponent(m_pSingletonComponent.Borrow());
  }
}

template <typename ComponentType>
EZ_ALWAYS_INLINE ComponentType* ezSettingsComponentManager<ComponentType>::GetSingletonComponent()
{
  return m_pSingletonComponent.Borrow();
}

template <typename ComponentType>
EZ_ALWAYS_INLINE const ComponentType* ezSettingsComponentManager<ComponentType>::GetSingletonComponent() const
{
  return m_pSingletonComponent.Borrow();
}

//static
template <typename ComponentType>
EZ_ALWAYS_INLINE ezUInt16 ezSettingsComponentManager<ComponentType>::TypeId()
{
  return ComponentType::TypeId();
}

template <typename ComponentType>
void ezSettingsComponentManager<ComponentType>::CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_AllComponents)
{
  out_AllComponents.PushBack(m_pSingletonComponent->GetHandle());
}

template <typename ComponentType>
void ezSettingsComponentManager<ComponentType>::CollectAllComponents(ezDynamicArray<ezComponent*>& out_AllComponents)
{
  out_AllComponents.PushBack(m_pSingletonComponent.Borrow());
}

template <typename ComponentType>
ezComponent* ezSettingsComponentManager<ComponentType>::CreateComponentStorage()
{
  if (m_pSingletonComponent != nullptr)
  {
    ezLog::Error("A component of type '{0}' is already present in this world. Having more than one is not allowed.", ezGetStaticRTTI<ComponentType>()->GetTypeName());
    return nullptr;
  }

  m_pSingletonComponent = EZ_NEW(GetAllocator(), ComponentType);

  return m_pSingletonComponent.Borrow();
}

template <typename ComponentType>
void ezSettingsComponentManager<ComponentType>::DeleteComponentStorage(ezComponent* pComponent, ezComponent*& out_pMovedComponent)
{
  out_pMovedComponent = m_pSingletonComponent.Borrow();
  m_pSingletonComponent.Reset();
}

