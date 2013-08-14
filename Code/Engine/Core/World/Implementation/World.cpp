#include <Core/PCH.h>

#include <Core/World/World.h>

ezStaticArray<ezWorld*, 64> ezWorld::s_Worlds;

ezWorld::ezWorld(const char* szWorldName) :
  m_Data(szWorldName)
{
  m_uiIndex = ezInvalidIndex;

  // find a free world slot
  for (ezUInt32 i = 0; i < s_Worlds.GetCount(); i++)
  {
    if (s_Worlds[i] == NULL)
    {
      s_Worlds[i] = this;
      m_uiIndex = i;
    }
  }

  if (m_uiIndex == ezInvalidIndex)
  {
    m_uiIndex = s_Worlds.GetCount();
    s_Worlds.PushBack(this);
  }
}

ezWorld::~ezWorld()
{
  s_Worlds[m_uiIndex] = NULL;
  m_uiIndex = ezInvalidIndex;
}

void ezWorld::CreateObjects(const ezArrayPtr<const ezGameObjectDesc>& descs, ezArrayPtr<ezGameObjectHandle> out_objects)
{
  EZ_ASSERT_API(out_objects.GetCount() == descs.GetCount(), "out_objects array must be the same size as descs array");

  const ezUInt32 uiCount = descs.GetCount();

  ezGameObjectHandle parent = descs[0].m_Parent;
  ezGameObject* pParentObject = NULL;
  ezGameObject::TransformationData* pParentData = NULL;
  ezUInt32 uiHierarchyLevel = 0;
  ezBitflags<ezObjectFlags> flags = descs[0].m_Flags;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)  
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    EZ_ASSERT(descs[i].m_Parent == parent, "all objects must have the same parent object");
    EZ_ASSERT(descs[i].m_Flags == flags, "all objects must be of the same type (static, dynamic, active)");
  }
#endif

  if (TryGetObject(parent, pParentObject))
  {
    pParentData = pParentObject->m_pTransformationData;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1;
    flags = pParentObject->m_Flags;
  }

  // get storage for the transformation data
  ezHybridArray<ezGameObject::TransformationData*, 32> TransformationData(&m_Data.m_Allocator);
  TransformationData.SetCount(uiCount);
  ezUInt32 uiTransformationDataIndex = m_Data.CreateTransformationData(flags, uiHierarchyLevel, TransformationData);

  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    const ezGameObjectDesc& desc = descs[i];

    ObjectStorageEntry storageEntry = m_Data.m_ObjectStorage.Create();
    ezGameObject* pNewObject = storageEntry.m_Ptr;

    // insert the new object into the id mapping table
    ezGameObjectId newId = m_Data.m_Objects.Insert(storageEntry);
    newId.m_WorldIndex = m_uiIndex;

    // fill out some data
    pNewObject->m_InternalId = newId;
    pNewObject->m_Flags = desc.m_Flags;
    pNewObject->m_uiPersistentId = 0; // todo
    pNewObject->m_Parent = parent;
    pNewObject->m_uiHierarchyLevel = uiHierarchyLevel;
    pNewObject->m_uiTransformationDataIndex = uiTransformationDataIndex + i;

    pNewObject->m_pWorld = this;

    if (!ezStringUtils::IsNullOrEmpty(desc.m_szName))
    {
      SetObjectName(newId, desc.m_szName);
    }
  
    // fill out the transformation data
    ezGameObject::TransformationData* pTransformationData = TransformationData[i];
    pTransformationData->m_pObject = pNewObject;
    pTransformationData->m_pParentData = pParentData;
    pTransformationData->m_localPosition = desc.m_LocalPosition.GetAsPositionVec4();
    pTransformationData->m_localRotation = desc.m_LocalRotation;
    pTransformationData->m_localScaling = desc.m_LocalScaling.GetAsDirectionVec4();
    
    // link the transformation data to the game object
    pNewObject->m_pTransformationData = pTransformationData;
  
    out_objects[i] = newId;
  }
}

void ezWorld::DeleteObjects(const ezArrayPtr<const ezGameObjectHandle>& objects)
{
  for (ezUInt32 i = 0; i < objects.GetCount(); ++i)
  {
    const ezGameObjectId id = objects[i].m_InternalId;

    ObjectStorageEntry storageEntry;
    if (!m_Data.m_Objects.TryGetValue(id, storageEntry))
      continue;

    ezGameObject* pObject = storageEntry.m_Ptr;
    pObject->m_InternalId = ezGameObjectId();
    pObject->m_Flags.Remove(ezObjectFlags::Active);
    m_Data.m_DeadObjects.PushBack(storageEntry);
    m_Data.m_Objects.Remove(id);

    /// \todo: fix parent etc

    // delete attached components
    const ezArrayPtr<ezComponentHandle> components = pObject->GetComponents();
    for (ezUInt32 c = 0; c < components.GetCount(); ++c)
    {
      const ezComponentHandle& component = components[c];
      const ezUInt16 uiTypeId = component.m_InternalId.m_TypeId;

      if (uiTypeId >= m_Data.m_ComponentManagers.GetCount())
        continue;
        
      if (ezComponentManagerBase* pManager = m_Data.m_ComponentManagers[uiTypeId])
      {
        pManager->DeleteComponent(component);
      }
    }

    // delete children
    for (ezGameObject::ChildIterator it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      DeleteObject(it->GetHandle());
    }
  }
}

bool ezWorld::TryGetObject(ezUInt64 uiPersistentId, ezGameObject*& out_pObject) const
{
  ezGameObjectId internalId;
  if (m_Data.m_PersistentToInternalTable.TryGetValue(uiPersistentId, internalId))
  {
    out_pObject = m_Data.m_Objects[internalId].m_Ptr;
    return true;
  }
  return false;
}

bool ezWorld::TryGetObject(const char* szObjectName, ezGameObject*& out_pObject) const
{
  ezGameObjectId internalId;
  if (m_Data.m_NameToInternalTable.TryGetValue(szObjectName, internalId))
  {
    out_pObject = m_Data.m_Objects[internalId].m_Ptr;
    return true;
  }
  return false;
}

void ezWorld::Update()
{
  EZ_ASSERT(m_Data.m_UnresolvedUpdateFunctions.IsEmpty(), "There are update functions with unresolved depedencies.");
  
  // pre-async phase
  UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PreAsync]);

  // async phase
  UpdateAsynchronous();

  // post-async phase
  UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PostAsync]);

  DeleteDeadObjects();
  DeleteDeadComponents();

  UpdateWorldTransforms();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ezWorld::SetObjectName(ezGameObjectId internalId, const char* szName)
{
  m_Data.m_InternalToNameTable.Insert(internalId, szName);
  const char* szInsertedName = m_Data.m_InternalToNameTable[internalId].GetData();
  m_Data.m_NameToInternalTable.Insert(szInsertedName, internalId);
}

const char* ezWorld::GetObjectName(ezGameObjectId internalId) const
{
  ezString name;
  if (m_Data.m_InternalToNameTable.TryGetValue(internalId, name))
  {
    return name.GetData();
  }
  return NULL;
}

void ezWorld::QueueMessage(ezMessage& msg, ezBitflags<ezGameObject::MsgRouting> routing, const ezGameObjectHandle& senderObject)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ezResult ezWorld::RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  EZ_ASSERT(desc.m_Phase == ezComponentManagerBase::UpdateFunctionDesc::Async || desc.m_uiGranularity == 0, "Granularity must be 0 for synchronous update functions");

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];

  if (desc.m_DependsOn.IsEmpty())
  {
    ezInternal::WorldData::RegisteredUpdateFunction newFunction;
    newFunction.m_Function = desc.m_Function;
    newFunction.m_uiGranularity = desc.m_uiGranularity;

    updateFunctions.PushBack(newFunction);
  }
  else
  {
    EZ_ASSERT(desc.m_Phase != ezComponentManagerBase::UpdateFunctionDesc::Async, "Asynchronous update functions must not have dependencies");

    if (RegisterUpdateFunctionWithDependency(desc, true) == EZ_FAILURE)
      return EZ_FAILURE;
  }

  // new function was registered successfully, try to insert unresolved functions
  for (ezUInt32 i = 0; i < m_Data.m_UnresolvedUpdateFunctions.GetCount(); ++i)
  {
    RegisterUpdateFunctionWithDependency(m_Data.m_UnresolvedUpdateFunctions[i], false);
  }

  return EZ_SUCCESS;
}

ezResult ezWorld::RegisterUpdateFunctionWithDependency(const ezComponentManagerBase::UpdateFunctionDesc& desc, 
  bool bInsertAsUnresolved)
{
  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];
  ezUInt32 uiInsertionIndex = 0;

  for (ezUInt32 i = 0; i < desc.m_DependsOn.GetCount(); ++i)
  {
    ezUInt32 uiDependencyIndex = ezInvalidIndex;

    // search for dependency from back so we get the last index if the same update function is used multiple times
    for (ezUInt32 j = updateFunctions.GetCount(); j-- > 0;)
    {
      if (updateFunctions[j].m_Function == desc.m_DependsOn[i])
      {
        uiDependencyIndex = j;
        break;
      }
    }

    if (uiDependencyIndex == ezInvalidIndex) // dependency not found
    {
      m_Data.m_UnresolvedUpdateFunctions.PushBack(desc);
      return EZ_FAILURE;
    }
    else
    {
      uiInsertionIndex = ezMath::Max(uiInsertionIndex, uiDependencyIndex);
    }
  }

  ezInternal::WorldData::RegisteredUpdateFunction newFunction;
  newFunction.m_Function = desc.m_Function;
  newFunction.m_uiGranularity = desc.m_uiGranularity;

  updateFunctions.Insert(newFunction, uiInsertionIndex);

  return EZ_SUCCESS;
}

ezResult ezWorld::DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  ezResult result = EZ_FAILURE;

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];

  for (ezUInt32 i = 0; i < updateFunctions.GetCount(); ++i)
  {
    if (updateFunctions[i].m_Function == desc.m_Function)
    {
      updateFunctions.RemoveAt(i);
      --i;
      result = EZ_SUCCESS;
    }
  }

  return result;
}

void ezWorld::UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions)
{
  for (ezUInt32 i = 0; i < updateFunctions.GetCount(); ++i)
  {
    ezInternal::WorldData::RegisteredUpdateFunction& updateFunction = updateFunctions[i];
    ezComponentManagerBase* pManager = static_cast<ezComponentManagerBase*>(updateFunction.m_Function.GetInstance());

    updateFunction.m_Function(0, pManager->GetActiveComponentCount());
  }
}

void ezWorld::UpdateAsynchronous()
{
  ezTaskGroupID taskGroupId = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = 
    m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Async];

  ezUInt32 uiCurrentTaskIndex = 0;

  for (ezUInt32 i = 0; i < updateFunctions.GetCount(); ++i)
  {
    ezInternal::WorldData::RegisteredUpdateFunction& updateFunction = updateFunctions[i];
    ezComponentManagerBase* pManager = static_cast<ezComponentManagerBase*>(updateFunction.m_Function.GetInstance());

    const ezUInt32 uiTotalCount = pManager->GetActiveComponentCount();
    ezUInt32 uiStartIndex = 0;
    ezUInt32 uiGranularity = (updateFunction.m_uiGranularity != 0) ? updateFunction.m_uiGranularity : uiTotalCount;

    while (uiStartIndex < uiTotalCount)
    {
      ezInternal::WorldData::UpdateTask* pTask;
      if (uiCurrentTaskIndex < m_Data.m_UpdateTasks.GetCount())
      {
        pTask = m_Data.m_UpdateTasks[uiCurrentTaskIndex];
      }
      else
      {
        pTask = EZ_NEW(&m_Data.m_Allocator, ezInternal::WorldData::UpdateTask)();
        m_Data.m_UpdateTasks.PushBack(pTask);
      }
            
      pTask->SetTaskName("Update Task"); /// \todo: get name from delegate
      pTask->m_Function = updateFunction.m_Function;
      pTask->m_uiStartIndex = uiStartIndex;
      pTask->m_uiCount = ezMath::Min(uiGranularity, uiTotalCount - uiStartIndex); ///  \todo: set count to invalidindex for last task

      ++uiCurrentTaskIndex;
      uiStartIndex += uiGranularity;
    }
  }

  ezTaskSystem::StartTaskGroup(taskGroupId);
  ezTaskSystem::WaitForGroup(taskGroupId);
}

void ezWorld::DeleteDeadObjects()
{
  // Sort the dead objects by index and then delete them backwards so lower indices stay valid
  m_Data.m_DeadObjects.Sort();

  for (ezUInt32 i = m_Data.m_DeadObjects.GetCount(); i-- > 0; )
  {
    ObjectStorageEntry entry = m_Data.m_DeadObjects[i];
    ezGameObject* pObject = entry.m_Ptr;

    m_Data.DeleteTransformationData(pObject->m_Flags, pObject->m_uiHierarchyLevel, 
      pObject->m_uiTransformationDataIndex);
    m_Data.m_ObjectStorage.Delete(entry);
    
    // patch the id table: the last element in the storage has been moved to deleted object's location, 
    // thus the pointer now points to another object
    ezGameObjectId id = entry.m_Ptr->m_InternalId;
    if (id.m_InstanceIndex != ezGameObjectId::INVALID_INSTANCE_INDEX)
      m_Data.m_Objects[id] = entry;
  }

  m_Data.m_DeadObjects.Clear();
}

void ezWorld::DeleteDeadComponents()
{
  // Sort the dead components by index and then delete them backwards so lower indices stay valid
  m_Data.m_DeadComponents.Sort();

  for (ezUInt32 i = m_Data.m_DeadComponents.GetCount(); i-- > 0; )
  {
    ezComponentManagerBase::ComponentStorageEntry entry = m_Data.m_DeadComponents[i];
    ezComponentManagerBase* pManager = m_Data.m_ComponentManagers[entry.m_Ptr->GetTypeId()];
    pManager->DeleteDeadComponent(entry);
  }

  m_Data.m_DeadComponents.Clear();
}

void ezWorld::UpdateWorldTransforms()
{
}
