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

void ezWorld::CreateObjects(ezArrayPtr<ezGameObjectHandle> out_objects, 
  const ezArrayPtr<const ezGameObjectDesc>& descs, const ezGameObjectHandle& parent)
{
  EZ_ASSERT_API(out_objects.GetCount() == descs.GetCount(), "out_objects array must be the same size as descs array");

  const ezUInt32 uiCount = descs.GetCount();

  ezGameObject* pParentObject = NULL;
  ezGameObject::HierarchicalData* pParentData = NULL;
  ezUInt32 uiHierarchyLevel = 0;
  ezBitflags<ezObjectFlags> flags = descs[0].m_Flags;

  if (pParentObject = GetObject(parent))
  {
    pParentData = pParentObject->m_pHierarchicalData;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1;
    flags = pParentObject->m_Flags;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)  
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    EZ_ASSERT(descs[i].m_Flags == flags, "all objects must be of the same type (static, dynamic, active)");
  }
#endif

  // get storage for the hierarchical data
  ezHybridArray<ezGameObject::HierarchicalData*, 32> hierarchicalData(&m_Data.m_Allocator);
  hierarchicalData.SetCount(uiCount);
  ezUInt32 uiHierarchicalDataIndex = m_Data.CreateHierarchicalData(flags, uiHierarchyLevel, hierarchicalData);

  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    const ezGameObjectDesc& desc = descs[i];

    ezGameObject* pNewObject = m_Data.m_ObjectStorage.Create();

    // insert the new object into the id mapping table
    ezGameObjectId newId = m_Data.m_Objects.Insert(pNewObject);
    newId.m_WorldIndex = m_uiIndex;

    // construct the new object and fill out some data
    ezMemoryUtils::Construct(pNewObject, 1);
    pNewObject->m_InternalId = newId;
    pNewObject->m_Flags = desc.m_Flags;
    pNewObject->m_uiPersistentId = 0; // todo
    pNewObject->m_Parent = parent;
    pNewObject->m_uiHierarchyLevel = uiHierarchyLevel;
    pNewObject->m_uiHierarchicalDataIndex = uiHierarchicalDataIndex + i;

    pNewObject->m_pWorld = this;

    if (!ezStringUtils::IsNullOrEmpty(desc.m_szName))
    {
      SetObjectName(newId, desc.m_szName);
    }
  
    // fill out the hierarchical data
    ezGameObject::HierarchicalData* pHierarchicalData = hierarchicalData[i];
    pHierarchicalData->m_pObject = pNewObject;
    pHierarchicalData->m_pParentData = pParentData;
    pHierarchicalData->m_localPosition = desc.m_LocalPosition.GetAsPositionVec4();
    pHierarchicalData->m_localRotation = desc.m_LocalRotation;
    pHierarchicalData->m_localScaling = desc.m_LocalScaling.GetAsDirectionVec4();
    
    // link the hierarchical data to the game object
    pNewObject->m_pHierarchicalData = pHierarchicalData;
  
    out_objects[i] = newId;
  }
}

void ezWorld::DeleteObjects(const ezArrayPtr<const ezGameObjectHandle>& objects)
{
  for (ezUInt32 i = 0; i < objects.GetCount(); ++i)
  {
    const ezGameObjectId id = objects[i].m_InternalId;

    ezGameObject* pObject = NULL;
    if (!m_Data.m_Objects.TryGetValue(id, pObject))
      continue;

    pObject->m_InternalId = ezGameObjectId();
    pObject->m_Flags.Remove(ezObjectFlags::Active);
    m_Data.m_DeadObjects.PushBack(pObject);
    m_Data.m_Objects.Remove(id);

    // todo: handle children, fix parent etc

    ezArrayPtr<ezComponentHandle> components = pObject->GetComponents();
    for (ezUInt32 c = 0; c < components.GetCount(); ++c)
    {
      const ezUInt16 uiTypeId = components[i].m_InternalId.m_TypeId;

      if (uiTypeId >= m_Data.m_ComponentManagers.GetCount())
        continue;
        
      if (ezComponentManagerBase* pManager = m_Data.m_ComponentManagers[uiTypeId])
      {
        pManager->DeleteComponent(components[i]);
      }
    }
  }
}

ezGameObject* ezWorld::GetObject(ezUInt64 uiPersistentId) const
{
  ezGameObjectId internalId;
  if (m_Data.m_PersistentToInternalTable.TryGetValue(uiPersistentId, internalId))
  {
    ezGameObject* pObject = NULL;
    m_Data.m_Objects.TryGetValue(internalId, pObject);
    return pObject;
  }
  return NULL;
}

ezGameObject* ezWorld::GetObject(const char* szObjectName) const
{
  ezGameObjectId internalId;
  if (m_Data.m_NameToInternalTable.TryGetValue(szObjectName, internalId))
  {
    ezGameObject* pObject = NULL;
    m_Data.m_Objects.TryGetValue(internalId, pObject);
    return pObject;
  }
  return NULL;
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

    updateFunction.m_Function(0, pManager->GetComponentCount());
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

    const ezUInt32 uiTotalCount = pManager->GetComponentCount();
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
            
      pTask->SetTaskName("Update Task"); // todo: get name from delegate
      pTask->m_Function = updateFunction.m_Function;
      pTask->m_uiStartIndex = uiStartIndex;
      pTask->m_uiCount = ezMath::Min(uiGranularity, uiTotalCount - uiStartIndex);

      ++uiCurrentTaskIndex;
      uiStartIndex += uiGranularity;
    }
  }

  ezTaskSystem::StartTaskGroup(taskGroupId);
  ezTaskSystem::WaitForGroup(taskGroupId);
}

void ezWorld::DeleteDeadObjects()
{
}

void ezWorld::UpdateWorldTransforms()
{
}
