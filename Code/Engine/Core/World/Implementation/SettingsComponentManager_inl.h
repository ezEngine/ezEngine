
template <typename ComponentType>
ezSettingsComponentManager<ComponentType>::ezSettingsComponentManager(ezWorld* pWorld)
    : ezComponentManagerBase(pWorld)
{
}

template <typename ComponentType>
ezSettingsComponentManager<ComponentType>::~ezSettingsComponentManager()
{
  for (auto& component : m_Components)
  {
    DeinitializeComponent(component.Borrow());
  }
}

template <typename ComponentType>
EZ_ALWAYS_INLINE ComponentType* ezSettingsComponentManager<ComponentType>::GetSingletonComponent()
{
  if (!m_Components.IsEmpty())
  {
    return m_Components[0].Borrow();
  }

  return nullptr;
}

template <typename ComponentType>
EZ_ALWAYS_INLINE const ComponentType* ezSettingsComponentManager<ComponentType>::GetSingletonComponent() const
{
  if (!m_Components.IsEmpty())
  {
    return m_Components[0].Borrow();
  }

  return nullptr;
}

// static
template <typename ComponentType>
EZ_ALWAYS_INLINE ezUInt16 ezSettingsComponentManager<ComponentType>::TypeId()
{
  return ComponentType::TypeId();
}

template <typename ComponentType>
void ezSettingsComponentManager<ComponentType>::CollectAllComponents(ezDynamicArray<ezComponentHandle>& out_AllComponents)
{
  for (auto& component : m_Components)
  {
    out_AllComponents.PushBack(component->GetHandle());
  }
}

template <typename ComponentType>
void ezSettingsComponentManager<ComponentType>::CollectAllComponents(ezDynamicArray<ezComponent*>& out_AllComponents)
{
  for (auto& component : m_Components)
  {
    out_AllComponents.PushBack(component.Borrow());
  }
}

template <typename ComponentType>
ezComponent* ezSettingsComponentManager<ComponentType>::CreateComponentStorage()
{
  if (!m_Components.IsEmpty())
  {
    ezLog::Warning("A component of type '{0}' is already present in this world. Having more than one is not allowed.",
                   ezGetStaticRTTI<ComponentType>()->GetTypeName());
  }

  m_Components.PushBack(EZ_NEW(GetAllocator(), ComponentType));
  return m_Components.PeekBack().Borrow();
}

template <typename ComponentType>
void ezSettingsComponentManager<ComponentType>::DeleteComponentStorage(ezComponent* pComponent, ezComponent*& out_pMovedComponent)
{
  out_pMovedComponent = pComponent;

  for (ezUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    if (m_Components[i].Borrow() == pComponent)
    {
      m_Components.RemoveAtAndCopy(i);
      break;
    }
  }
}
