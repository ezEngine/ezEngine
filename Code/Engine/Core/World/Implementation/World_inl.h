
EZ_FORCE_INLINE const char* ezWorld::GetName() const
{ 
  return m_Data.m_Name.GetData(); 
}

EZ_FORCE_INLINE ezGameObjectHandle ezWorld::CreateObject(const ezGameObjectDesc& desc)
{
  ezGameObjectHandle newObject;
  CreateObjects(ezArrayPtr<const ezGameObjectDesc>(&desc, 1), ezArrayPtr<ezGameObjectHandle>(&newObject, 1));
  return newObject;
}

EZ_FORCE_INLINE ezGameObjectHandle ezWorld::CreateObject(const ezGameObjectDesc& desc, ezGameObject*& out_pObject)
{
  ezGameObjectHandle newObject;
  CreateObjects(ezArrayPtr<const ezGameObjectDesc>(&desc, 1), ezArrayPtr<ezGameObjectHandle>(&newObject, 1));
  out_pObject = m_Data.m_Objects[newObject].m_Ptr;
  return newObject;
}

inline void ezWorld::CreateObjects(const ezArrayPtr<const ezGameObjectDesc>& descs, ezArrayPtr<ezGameObjectHandle> out_objects, 
  ezArrayPtr<ezGameObject*> out_pObjects)
{
  CreateObjects(descs, out_objects);
  for (ezUInt32 i = 0; i < out_objects.GetCount(); ++i)
  {
    out_pObjects[i] = m_Data.m_Objects[out_objects[i]].m_Ptr;
  }
}

EZ_FORCE_INLINE void ezWorld::DeleteObject(const ezGameObjectHandle& object)
{
  DeleteObjects(ezArrayPtr<const ezGameObjectHandle>(&object, 1));
}

EZ_FORCE_INLINE bool ezWorld::IsValidObject(const ezGameObjectHandle& object) const
{
  CheckForMultithreadedAccess();
  EZ_ASSERT(object.m_InternalId.m_WorldIndex == m_uiIndex, 
    "Object does not belong to this world. Expected world id %d got id %d", m_uiIndex, object.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.Contains(object);
}

EZ_FORCE_INLINE bool ezWorld::TryGetObject(const ezGameObjectHandle& object, ezGameObject*& out_pObject) const
{
  CheckForMultithreadedAccess();
  EZ_ASSERT(object.m_InternalId.m_WorldIndex == m_uiIndex, 
    "Object does not belong to this world. Expected world id %d got id %d", m_uiIndex, object.m_InternalId.m_WorldIndex);

  ObjectStorageEntry storageEntry = { NULL };
  bool bResult = m_Data.m_Objects.TryGetValue(object, storageEntry);
  out_pObject = storageEntry.m_Ptr;
  return bResult;
}

EZ_FORCE_INLINE ezUInt32 ezWorld::GetObjectCount() const
{
  return m_Data.m_Objects.GetCount();
}

template <typename ManagerType>
ManagerType* ezWorld::CreateComponentManager()
{
  CheckForMultithreadedAccess();
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponentManagerBase, ManagerType), 
    "Not a valid component manager type");

  const ezUInt16 uiTypeId = ManagerType::TypeId();
  if (uiTypeId >= m_Data.m_ComponentManagers.GetCount())
  {
    m_Data.m_ComponentManagers.SetCount(uiTypeId + 1);
  }

  ManagerType* pManager = static_cast<ManagerType*>(m_Data.m_ComponentManagers[uiTypeId]);
  if (pManager == NULL)
  {
    pManager = EZ_NEW(&m_Data.m_Allocator, ManagerType)(this);
    pManager->Initialize();
    
    m_Data.m_ComponentManagers[uiTypeId] = pManager;
  }

  return pManager;
}

template <typename ManagerType>
EZ_FORCE_INLINE ManagerType* ezWorld::GetComponentManager() const
{
  CheckForMultithreadedAccess();
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponentManagerBase, ManagerType), 
    "Not a valid component manager type");

  const ezUInt16 uiTypeId = ManagerType::TypeId();
  ManagerType* pManager = NULL;
  if (uiTypeId < m_Data.m_ComponentManagers.GetCount())
  {
    pManager = static_cast<ManagerType*>(m_Data.m_ComponentManagers[uiTypeId]);
  }

  EZ_ASSERT(pManager != NULL, "Component Manager (id: %u) does not exists.", uiTypeId); /// \todo use RTTI to print a useful name
  return pManager;
}

inline bool ezWorld::IsValidComponent(const ezComponentHandle& component) const
{
  CheckForMultithreadedAccess();
  const ezUInt16 uiTypeId = component.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_ComponentManagers.GetCount())
  {
    if (ezComponentManagerBase* pManager = m_Data.m_ComponentManagers[uiTypeId])
    {
      return pManager->IsValidComponent(component);
    }
  }

  return false;
}

//static
template <typename ComponentType>
EZ_FORCE_INLINE bool ezWorld::IsComponentOfType(const ezComponentHandle& component)
{
  return ezComponentManagerBase::IsComponentOfType<ComponentType>(component);
}

template <typename ComponentType>
inline bool ezWorld::TryGetComponent(const ezComponentHandle& component, ComponentType*& out_pComponent) const
{
  CheckForMultithreadedAccess();
  EZ_CHECK_AT_COMPILETIME_MSG(EZ_IS_DERIVED_FROM_STATIC(ezComponent, ComponentType),
    "Not a valid component type");

  EZ_ASSERT(IsComponentOfType<ComponentType>(component),
    "The given component handle is not of the expected type. Expected type id %d, got type id %d",
    ComponentType::TypeId(), component.m_InternalId.m_TypeId);

  const ezUInt16 uiTypeId = component.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_ComponentManagers.GetCount())
  {
    if (ezComponentManagerBase* pManager = m_Data.m_ComponentManagers[uiTypeId])
    {
      ezComponent* pComponent = NULL;
      bool bResult = pManager->TryGetComponent(component, pComponent);
      out_pComponent = static_cast<ComponentType*>(pComponent);
      return bResult;
    }
  }

  return false;
}

EZ_FORCE_INLINE ezIAllocator* ezWorld::GetAllocator()
{
  return &m_Data.m_Allocator;
}

EZ_FORCE_INLINE ezLargeBlockAllocator* ezWorld::GetBlockAllocator()
{
  return &m_Data.m_BlockAllocator;
}

EZ_FORCE_INLINE void ezWorld::SetUserData(void* pUserData)
{
  m_Data.m_pUserData = pUserData;
}

EZ_FORCE_INLINE void* ezWorld::GetUserData() const
{
  return m_Data.m_pUserData;
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

EZ_FORCE_INLINE void ezWorld::CheckForMultithreadedAccess() const
{
  EZ_ASSERT(!m_Data.m_bIsInAsyncPhase && m_Data.m_ThreadHandle == ezThreadUtils::GetCurrentThreadHandle(), \
    "World must not be accessed while in async update phase or from another thread than the creation thread.");
}

EZ_FORCE_INLINE void ezWorld::HandleMessage(ezGameObject* pReceiverObject, ezMessage& msg, ezBitflags<ezObjectMsgRouting> routing)
{
  CheckForMultithreadedAccess();

  ++m_Data.m_uiHandledMessageCounter;
  pReceiverObject->OnMessage(msg, routing);
}

EZ_FORCE_INLINE ezUInt32  ezWorld::GetHandledMessageCounter() const
{
  return m_Data.m_uiHandledMessageCounter;
}
