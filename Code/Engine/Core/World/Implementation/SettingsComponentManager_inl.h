
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
