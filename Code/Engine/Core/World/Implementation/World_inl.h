
EZ_FORCE_INLINE const char* ezWorld::GetName() const
{ 
  return m_Name.GetData(); 
}

EZ_FORCE_INLINE ezGameObjectHandle ezWorld::CreateObject(const ezGameObjectDesc& desc, 
  const ezGameObjectHandle& parent)
{
  ezGameObjectHandle newObject;
  CreateObjects(ezArrayPtr<ezGameObjectHandle>(&newObject, 1), ezArrayPtr<const ezGameObjectDesc>(&desc, 1), parent);
  return newObject;
}

EZ_FORCE_INLINE void ezWorld::DeleteObject(const ezGameObjectHandle& object)
{
  DeleteObjects(ezArrayPtr<const ezGameObjectHandle>(&object, 1));
}

EZ_FORCE_INLINE bool ezWorld::IsValidObject(const ezGameObjectHandle& object) const
{
  EZ_ASSERT(object.m_InternalId.m_WorldIndex == m_uiIndex, 
    "Object does not belong to this world. Expected world id %d got id %d", m_uiIndex, object.m_InternalId.m_WorldIndex);

  return m_Objects.Contains(object.m_InternalId);
}

EZ_FORCE_INLINE ezGameObject* ezWorld::GetObject(const ezGameObjectHandle& object) const
{
  EZ_ASSERT(object.m_InternalId.m_WorldIndex == m_uiIndex, 
    "Object does not belong to this world. Expected world id %d got id %d", m_uiIndex, object.m_InternalId.m_WorldIndex);

  ezGameObject* pObject = NULL;
  m_Objects.TryGetValue(object.m_InternalId, pObject);
  return pObject;
}

EZ_FORCE_INLINE ezUInt32 ezWorld::GetObjectCount() const
{
  return m_Objects.GetCount();
}

template <typename ManagerType>
ManagerType* ezWorld::CreateComponentManager()
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponentManagerBase, ManagerType), 
    "Not a valid component manager type");

  const ezUInt16 uiTypeId = ManagerType::ComponentType::TypeId();
  while (uiTypeId >= m_ComponentManagers.GetCount())
  {
    m_ComponentManagers.PushBack(NULL);
  }

  ManagerType* pManager = static_cast<ManagerType*>(m_ComponentManagers[uiTypeId]);
  if (pManager == NULL)
  {
    pManager = EZ_NEW(&m_Allocator, ManagerType)(this);
    pManager->Initialize();
    
    m_ComponentManagers[uiTypeId] = pManager;
  }

  return pManager;
}

template <typename ManagerType>
EZ_FORCE_INLINE ManagerType* ezWorld::GetComponentManager() const
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponentManagerBase, ManagerType), 
    "Not a valid component manager type");

  const ezUInt16 uiTypeId = ManagerType::ComponentType::TypeId();
  if (uiTypeId < m_ComponentManagers.GetCount())
  {
    return static_cast<ManagerType*>(m_ComponentManagers[uiTypeId]);
  }

  return NULL;
}

inline bool ezWorld::IsValidComponent(const ezComponentHandle& component) const
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return false;
}
  
template <typename ComponentType>
EZ_FORCE_INLINE bool ezWorld::IsComponentOfType(const ezComponentHandle& component) const
{
  return component.m_InternalId.m_TypeId == ComponentType::TypeId();
}

template <typename ComponentType>
EZ_FORCE_INLINE ComponentType* ezWorld::GetComponent(const ezComponentHandle& component) const
{
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponent, ComponentType), 
    "Not a valid component type");

  EZ_ASSERT(IsComponentOfType<ComponentType>(), 
    "The given component handle is not of the expected type. Expected type id %d, got type id %d",
    ComponentType::TypeId(), component.m_InternalId.m_TypeId);

  return static_cast<ComponentType*>(GetComponent(component));
}

inline ezComponent* ezWorld::GetComponent(const ezComponentHandle& component) const
{
  const ezUInt16 uiTypeId = component.m_InternalId.m_TypeId;

  if (uiTypeId < m_ComponentManagers.GetCount())
  {
    ezComponentManagerBase* pManager = m_ComponentManagers[uiTypeId];
    if (pManager != NULL)
    {
      return pManager->GetComponent(component);
    }
  }

  return NULL;
}

EZ_FORCE_INLINE ezIAllocator* ezWorld::GetAllocator()
{
  return &m_Allocator;
}

EZ_FORCE_INLINE ezLargeBlockAllocator* ezWorld::GetBlockAllocator()
{
  return &m_BlockAllocator;
}

//static
EZ_FORCE_INLINE ezUInt32 ezWorld::GetWorldCount()
{
  return s_Worlds.GetCount();
}

//static
EZ_FORCE_INLINE ezWorld* ezWorld::GetWorld(ezUInt32 uiIndex)
{
  return s_Worlds[uiIndex];
}
