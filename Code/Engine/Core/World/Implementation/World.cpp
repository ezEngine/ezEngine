#include <Core/PCH.h>

#include <Core/World/World.h>

ezStaticArray<ezWorld*, 64> ezWorld::s_Worlds;

ezWorld::ezWorld(const char* szWorldName) :
  m_Name(szWorldName),
  m_Allocator(m_Name.GetData(), ezFoundation::GetDefaultAllocator()),
  m_AllocatorWrapper(&m_Allocator),
  m_BlockAllocator(m_Name.GetData(), &m_Allocator),
  m_ObjectStorage(&m_BlockAllocator, &m_Allocator)
{
  m_AllocatorWrapper.Reset();

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

  for (ezUInt32 i = 0; i < HIERARCHY_COUNT; ++i)
  {
    m_hierarchies[i].m_pWorld = this;
  }

  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject::HierarchicalData) == 128);
  EZ_CHECK_AT_COMPILETIME(sizeof(ezGameObject) == 128);
}

ezWorld::~ezWorld()
{
  // delete all component manager
  for (ezUInt32 i = 0; i < m_ComponentManagers.GetCount(); ++i)
  {
    EZ_DELETE(&m_Allocator, m_ComponentManagers[i]);
  }

  s_Worlds[m_uiIndex] = NULL;
}

void ezWorld::CreateObjects(ezArrayPtr<ezGameObjectHandle> out_objects, 
  const ezArrayPtr<const ezGameObjectDesc>& descs, const ezGameObjectHandle& parent)
{
  EZ_ASSERT_API(out_objects.GetCount() == descs.GetCount(), "out_objects array must be the same size as descs array");

  const ezUInt32 uiCount = descs.GetCount();

  ezGameObject* pParentObject = NULL;
  ezGameObject::HierarchicalData* pParentData = NULL;
  ezUInt32 uiHierarchyLevel = 0;
  ezBitflags<ezGameObjectFlags> flags = descs[0].m_Flags;

  if (pParentObject = GetObject(parent))
  {
    pParentData = pParentObject->m_pHierarchicalData;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1;
    flags = pParentObject->m_Flags;

    // update parent object
    pParentObject->m_uiChildCount += uiCount;
  }

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)  
  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    EZ_ASSERT(descs[i].m_Flags == flags, "all objects must be of the same type (static, dynamic, active)");
  }
#endif

  // get storage for the hierarchical data
  ezHybridArray<ezGameObject::HierarchicalData*, 32> hierarchicalData(&m_Allocator);
  hierarchicalData.SetCount(uiCount);

  HierarchyType type = GetHierarchyType(flags);
  ezUInt32 uiFirstIndex = m_hierarchies[type].ReserveData(hierarchicalData, uiHierarchyLevel, ezInvalidIndex);
  
  /*if (pParentObject != NULL)
  {
    pParentObject->m_uiFirstChildIndex = ezMath::Min(pParentObject->m_uiFirstChildIndex, uiFirstIndex);
  }*/

  for (ezUInt32 i = 0; i < uiCount; ++i)
  {
    const ezGameObjectDesc& desc = descs[i];

    ezGameObject* pNewObject = m_ObjectStorage.Create();

    // insert the new object into the id mapping table
    ezGameObjectId newId = m_Objects.Insert(pNewObject);
    newId.m_WorldIndex = m_uiIndex;

    // construct the new object and fill out some data
    ezMemoryUtils::Construct(pNewObject, 1);
    pNewObject->m_InternalId = newId;
    pNewObject->m_Flags = desc.m_Flags;
    pNewObject->m_uiPersistentId = 0; // todo
    pNewObject->m_Parent = parent;
    pNewObject->m_FirstChild = ezGameObjectHandle();
    pNewObject->m_uiChildCount = 0;
    pNewObject->m_uiHierarchyLevel = uiHierarchyLevel;

    pNewObject->m_pWorld = this;
  
    // fill out the hierarchical data
    ezGameObject::HierarchicalData* pHierarchicalData = hierarchicalData[i];
    pHierarchicalData->m_internalId = newId;
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
    if (m_Objects.TryGetValue(id, pObject))
    {
      m_DeadObjects.PushBack(pObject);
      m_Objects.Remove(id);

      // todo: handle children, fix parent etc
    }
  }
}

ezGameObject* ezWorld::GetObject(ezUInt64 uiPersistentId) const
{
  ezGameObjectId internalId;
  if (m_PersistentToInternalTable.TryGetValue(uiPersistentId, internalId))
  {
    ezGameObject* pObject = NULL;
    m_Objects.TryGetValue(internalId, pObject);
    return pObject;
  }
  return NULL;
}

ezGameObject* ezWorld::GetObject(const char* szObjectName) const
{
  ezGameObjectId internalId;
  if (m_NameToInternalTable.TryGetValue(szObjectName, internalId))
  {
    ezGameObject* pObject = NULL;
    m_Objects.TryGetValue(internalId, pObject);
    return pObject;
  }
  return NULL;
}

void ezWorld::Update()
{
  EZ_ASSERT(m_UnresolvedUpdateFunctions.IsEmpty(), "There are update functions with unresolved depedencies.");
  
  // pre-async phase
  UpdateSynchronous(m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PreAsync]);

  // async phase
  UpdateAsynchronous();

  // post-async phase
  UpdateSynchronous(m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PostAsync]);

  DeleteDeadObjects();

  UpdateWorldTransforms();
}

void ezWorld::GetMemStats(ezIAllocator::Stats& stats) const
{
  m_Allocator.GetStats(stats);

  ezIAllocator::Stats blockStats;
  m_BlockAllocator.GetStats(blockStats);

  stats.m_uiNumAllocations += blockStats.m_uiAllocationSize;
  stats.m_uiNumDeallocations += blockStats.m_uiNumDeallocations;
  stats.m_uiNumLiveAllocations += blockStats.m_uiNumLiveAllocations;
  stats.m_uiAllocationSize += blockStats.m_uiAllocationSize;
  stats.m_uiUsedMemorySize += blockStats.m_uiUsedMemorySize;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ezWorld::SetName(ezGameObjectId internalId, const char* szName)
{
  m_InternalToNameTable.Insert(internalId, szName);
  const char* szInsertedName = m_InternalToNameTable[internalId].GetData();
  m_NameToInternalTable.Insert(szInsertedName, internalId);
}

const char* ezWorld::GetName(ezGameObjectId internalId) const
{
  ezString name;
  if (m_InternalToNameTable.TryGetValue(internalId, name))
  {
    return name.GetData();
  }
  return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ezWorld::UpdateTask::Execute()
{
  m_Function(m_uiStartIndex, m_uiCount);
}

ezResult ezWorld::RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  EZ_ASSERT(desc.m_Phase == ezComponentManagerBase::UpdateFunctionDesc::Async || desc.m_uiGranularity == 0, "Granularity must be 0 for synchronous update functions");

  ezDynamicArrayBase<RegisteredUpdateFunction>& updateFunctions = m_UpdateFunctions[desc.m_Phase];

  if (desc.m_DependsOn.IsEmpty())
  {
    RegisteredUpdateFunction newFunction;
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
  for (ezUInt32 i = 0; i < m_UnresolvedUpdateFunctions.GetCount(); ++i)
  {
    RegisterUpdateFunctionWithDependency(m_UnresolvedUpdateFunctions[i], false);
  }

  return EZ_SUCCESS;
}

ezResult ezWorld::RegisterUpdateFunctionWithDependency(const ezComponentManagerBase::UpdateFunctionDesc& desc, 
  bool bInsertAsUnresolved)
{
  ezDynamicArrayBase<RegisteredUpdateFunction>& updateFunctions = m_UpdateFunctions[desc.m_Phase];
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
      m_UnresolvedUpdateFunctions.PushBack(desc);
      return EZ_FAILURE;
    }
    else
    {
      uiInsertionIndex = ezMath::Max(uiInsertionIndex, uiDependencyIndex);
    }
  }

  RegisteredUpdateFunction newFunction;
  newFunction.m_Function = desc.m_Function;
  newFunction.m_uiGranularity = desc.m_uiGranularity;

  updateFunctions.Insert(newFunction, uiInsertionIndex);

  return EZ_SUCCESS;
}

ezResult ezWorld::DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  ezResult result = EZ_FAILURE;

  ezDynamicArrayBase<RegisteredUpdateFunction>& updateFunctions = m_UpdateFunctions[desc.m_Phase];

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

void ezWorld::UpdateSynchronous(const ezArrayPtr<RegisteredUpdateFunction>& updateFunctions)
{
  for (ezUInt32 i = 0; i < updateFunctions.GetCount(); ++i)
  {
    RegisteredUpdateFunction& updateFunction = updateFunctions[i];
    ezComponentManagerBase* pManager = static_cast<ezComponentManagerBase*>(updateFunction.m_Function.GetInstance());

    updateFunction.m_Function(0, pManager->GetComponentCount());
  }
}

void ezWorld::UpdateAsynchronous()
{
  ezTaskGroupID taskGroupId = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

  ezDynamicArrayBase<RegisteredUpdateFunction>& updateFunctions = m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Async];

  ezUInt32 uiCurrentTaskIndex = 0;

  for (ezUInt32 i = 0; i < updateFunctions.GetCount(); ++i)
  {
    RegisteredUpdateFunction& updateFunction = updateFunctions[i];
    ezComponentManagerBase* pManager = static_cast<ezComponentManagerBase*>(updateFunction.m_Function.GetInstance());

    const ezUInt32 uiTotalCount = pManager->GetComponentCount();
    ezUInt32 uiStartIndex = 0;
    ezUInt32 uiGranularity = (updateFunction.m_uiGranularity != 0) ? updateFunction.m_uiGranularity : uiTotalCount;

    while (uiStartIndex < uiTotalCount)
    {
      UpdateTask* pTask;
      if (uiCurrentTaskIndex < m_UpdateTasks.GetCount())
      {
        pTask = m_UpdateTasks[uiCurrentTaskIndex];
      }
      else
      {
        pTask = EZ_NEW(&m_Allocator, UpdateTask)();
        m_UpdateTasks.PushBack(pTask);
      }
            
      pTask->SetTaskName("Update Task");
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
