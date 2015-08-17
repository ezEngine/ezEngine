#include <Core/PCH.h>

#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Time/Clock.h>

#include <Core/World/World.h>

static ezProfilingId s_PreAsyncProfilingID          = ezProfilingSystem::CreateId("Pre-Async Phase");
static ezProfilingId s_AsyncProfilingID             = ezProfilingSystem::CreateId("Async Phase");
static ezProfilingId s_PostAsyncProfilingID         = ezProfilingSystem::CreateId("Post-Async Phase");
static ezProfilingId s_UpdateTransformsProfilingID  = ezProfilingSystem::CreateId("Update Transforms");
static ezProfilingId s_PostTransformProfilingID     = ezProfilingSystem::CreateId("Post-Transform Phase");
static ezProfilingId s_DeleteDeadObjectsProfilingID = ezProfilingSystem::CreateId("Delete Dead Objects");

ezStaticArray<ezWorld*, 64> ezWorld::s_Worlds;

ezWorld::ezWorld(const char* szWorldName) :
  m_UpdateTask("", ezMakeDelegate(&ezWorld::UpdateFromThread, this)),
  m_Data(szWorldName)
{
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  ezStringBuilder sb = szWorldName;
  sb.Append(".Update");
  m_UpdateProfilingID = ezProfilingSystem::CreateId(sb.GetData());

  sb.Append(" Task");
  m_UpdateTask.SetTaskName(sb);

  m_uiIndex = ezInvalidIndex;

  // find a free world slot
  for (ezUInt32 i = 0; i < s_Worlds.GetCount(); i++)
  {
    if (s_Worlds[i] == nullptr)
    {
      s_Worlds[i] = this;
      m_uiIndex = i;
      break;
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
  // set all objects to inactive so components and children know that they shouldn't access the objects anymore.
  for (auto it = m_Data.m_ObjectStorage.GetIterator(); it.IsValid(); it.Next())
  {
    it->m_Flags.Remove(ezObjectFlags::Active);
  }

  // delete all component manager before we invalidate the world. Components can still access the world during deinitialization.
  for (ezUInt32 i = 0; i < m_Data.m_ComponentManagers.GetCount(); ++i)
  {
    if (ezComponentManagerBase* pManager = m_Data.m_ComponentManagers[i])
    {
      pManager->Deinitialize();
      EZ_DELETE(&m_Data.m_Allocator, pManager);
    }
  }
  m_Data.m_ComponentManagers.Clear();

  s_Worlds[m_uiIndex] = nullptr;
  m_uiIndex = ezInvalidIndex;
}

ezGameObjectHandle ezWorld::CreateObject(const ezGameObjectDesc& desc, ezGameObject*& out_pObject)
{
  CheckForWriteAccess();

  ezGameObject* pParentObject = nullptr;
  ezGameObject::TransformationData* pParentData = nullptr;
  ezUInt32 uiParentIndex = 0;
  ezUInt32 uiHierarchyLevel = 0;

  if (TryGetObject(desc.m_hParent, pParentObject))
  {
    pParentData = pParentObject->m_pTransformationData;
    uiParentIndex = desc.m_hParent.m_InternalId.m_InstanceIndex;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel;
    EZ_ASSERT_DEV(uiHierarchyLevel < (1 << 12), "Max hierarchy level reached");
    ++uiHierarchyLevel; // if there is a parent hierarchy level is parent level + 1
  }

  // get storage for the transformation data
  ezGameObject::TransformationData* pTransformationData = nullptr;
  ezUInt32 uiTransformationDataIndex = m_Data.CreateTransformationData(desc.m_Flags, uiHierarchyLevel, pTransformationData);

  // get storage for the object itself
  ObjectStorageEntry storageEntry = m_Data.m_ObjectStorage.Create();
  ezGameObject* pNewObject = storageEntry.m_Ptr;

  // insert the new object into the id mapping table
  ezGameObjectId newId = m_Data.m_Objects.Insert(storageEntry);
  newId.m_WorldIndex = m_uiIndex;

  // fill out some data
  pNewObject->m_InternalId = newId;
  pNewObject->m_Flags = desc.m_Flags;
  pNewObject->m_sName = desc.m_sName;
  pNewObject->m_pWorld = this;
  pNewObject->m_ParentIndex = uiParentIndex; 
  
  // fix links
  LinkToParent(pNewObject);

  pNewObject->m_uiHierarchyLevel = uiHierarchyLevel;
  pNewObject->m_uiTransformationDataIndex = uiTransformationDataIndex;

  // fill out the transformation data
  pTransformationData->m_pObject = pNewObject;
  pTransformationData->m_pParentData = pParentData;
  pTransformationData->m_localPosition = desc.m_LocalPosition.GetAsPositionVec4();
  pTransformationData->m_localRotation = desc.m_LocalRotation;
  pTransformationData->m_localScaling = desc.m_LocalScaling.GetAsDirectionVec4();
  pTransformationData->m_globalTransform.SetIdentity();
  pTransformationData->m_velocity.SetZero();
  pTransformationData->m_localBounds.SetInvalid();
  pTransformationData->m_globalBounds.SetInvalid();

  if (pParentData != nullptr)
  {
    pTransformationData->UpdateGlobalTransformWithParent();
  }
  else
  {
    pTransformationData->UpdateGlobalTransform();
  }   
    
  // link the transformation data to the game object
  pNewObject->m_pTransformationData = pTransformationData;

  out_pObject = pNewObject;
  return newId;
}

void ezWorld::DeleteObject(const ezGameObjectHandle& object)
{
  CheckForWriteAccess();

  ObjectStorageEntry storageEntry;
  if (!m_Data.m_Objects.TryGetValue(object, storageEntry))
    return;

  ezGameObject* pObject = storageEntry.m_Ptr;

  // set object to inactive so components and children know that they shouldn't access the object anymore.
  pObject->m_Flags.Remove(ezObjectFlags::Active);

  // delete children
  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteObject(it->GetHandle());
  }

  // delete attached components
  const ezArrayPtr<ezComponent*> components = pObject->m_Components;
  for (ezUInt32 c = 0; c < components.GetCount(); ++c)
  {
    ezComponent* pComponent = components[c];
    pComponent->GetManager()->DeleteComponent(pComponent->GetHandle());
  }
  EZ_ASSERT_DEV(pObject->m_Components.GetCount() == 0, "Components should already be removed");

  // fix parent and siblings
  UnlinkFromParent(pObject);

  // invalidate and remove from id table
  pObject->m_InternalId.Invalidate();
  m_Data.m_DeadObjects.PushBack(storageEntry);
  EZ_VERIFY(m_Data.m_Objects.Remove(object), "Implementation error.");
}

void ezWorld::DeleteObjectDelayed(const ezGameObjectHandle& hObject)
{
  ezDeleteObjectMessage msg;
  PostMessage(hObject, msg, ezObjectMsgQueueType::NextFrame, ezObjectMsgRouting::ToObjectOnly);
}

ezComponentManagerBase* ezWorld::GetComponentManager(const ezRTTI* pRtti) const
{
  CheckForReadAccess();

  for (auto pMan : m_Data.m_ComponentManagers)
  {
    if (pMan != nullptr && pMan->GetComponentType() == pRtti)
      return pMan;
  }

  return nullptr;
}

void ezWorld::PostMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg, 
  ezObjectMsgQueueType::Enum queueType, ezObjectMsgRouting::Enum routing)
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_ReceiverObject = receiverObject;
  metaData.m_Routing = routing;

  ezMessage* pMsgCopy = msg.Clone(ezFrameAllocator::GetCurrentAllocator());
  m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
}

void ezWorld::PostMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg, 
  ezObjectMsgQueueType::Enum queueType, ezTime delay, ezObjectMsgRouting::Enum routing)
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_ReceiverObject = receiverObject;
  metaData.m_Routing = routing;
  metaData.m_Due = ezClock::Get(ezGlobalClock_GameLogic)->GetAccumulatedTime() + delay;

  ezMessage* pMsgCopy = msg.Clone(&m_Data.m_Allocator);
  m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
}

void ezWorld::Update()
{
  CheckForWriteAccess();
  EZ_ASSERT_DEV(m_Data.m_UnresolvedUpdateFunctions.IsEmpty(), "There are update functions with unresolved dependencies.");

  EZ_LOG_BLOCK(m_Data.m_sName.GetData());
  EZ_PROFILE(m_UpdateProfilingID);

  // pre-async phase
  {
    EZ_PROFILE(s_PreAsyncProfilingID);
    ProcessQueuedMessages(ezObjectMsgQueueType::NextFrame);  
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PreAsync]);
  }

  // async phase
  {
    // remove write marker but keep the read marker. Thus no one can mark the world for writing now. Only reading is allowed in async phase.
    m_Data.m_WriteThreadID = (ezThreadID)0;

    EZ_PROFILE(s_AsyncProfilingID);
    UpdateAsynchronous();

    // restore write marker
    m_Data.m_WriteThreadID = ezThreadUtils::GetCurrentThreadID();
  }

  // post-async phase
  {
    EZ_PROFILE(s_PostAsyncProfilingID);
    ProcessQueuedMessages(ezObjectMsgQueueType::PostAsync);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PostAsync]);
  }

  // delete dead objects and update the object hierarchy
  {
    EZ_PROFILE(s_DeleteDeadObjectsProfilingID);
    DeleteDeadObjects();
    DeleteDeadComponents();
  }

  // update transforms
  {
    EZ_PROFILE(s_UpdateTransformsProfilingID);
    m_Data.UpdateGlobalTransforms();
  }

  // post-transform phase
  {
    EZ_PROFILE(s_PostTransformProfilingID);
    ProcessQueuedMessages(ezObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::PostTransform]);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ezWorld::SetParent(ezGameObject* pObject, ezGameObject* pNewParent)
{
  CheckForWriteAccess();

  if (GetObjectUnchecked(pObject->m_ParentIndex) == pNewParent)
    return;

  UnlinkFromParent(pObject);

  if (pNewParent != nullptr)
  {
    // Ensure that the parent's global transform is up-to-date otherwise the object's local transform will be wrong afterwards.
    pNewParent->UpdateGlobalTransform();

    pObject->m_ParentIndex = pNewParent->m_InternalId.m_InstanceIndex;
    LinkToParent(pObject);
  }

  PatchHierarchyData(pObject);
}

void ezWorld::LinkToParent(ezGameObject* pObject)
{
  if (ezGameObject* pParentObject = pObject->GetParent())
  {
    const ezUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (pParentObject->m_FirstChildIndex != 0)
    {
      pObject->m_PrevSiblingIndex = pParentObject->m_LastChildIndex;
      GetObjectUnchecked(pParentObject->m_LastChildIndex)->m_NextSiblingIndex = uiIndex;
    }
    else
    {
      pParentObject->m_FirstChildIndex = uiIndex;
    }

    pParentObject->m_LastChildIndex = uiIndex;
    pParentObject->m_ChildCount++;
  }
}

void ezWorld::UnlinkFromParent(ezGameObject* pObject)
{
  if (ezGameObject* pParentObject = pObject->GetParent())
  {
    const ezUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (uiIndex == pParentObject->m_FirstChildIndex)
      pParentObject->m_FirstChildIndex = pObject->m_NextSiblingIndex;

    if (uiIndex == pParentObject->m_LastChildIndex)
      pParentObject->m_LastChildIndex = pObject->m_PrevSiblingIndex;

    if (ezGameObject* pNextObject = GetObjectUnchecked(pObject->m_NextSiblingIndex))
      pNextObject->m_PrevSiblingIndex = pObject->m_PrevSiblingIndex;

    if (ezGameObject* pPrevObject = GetObjectUnchecked(pObject->m_PrevSiblingIndex))
      pPrevObject->m_NextSiblingIndex = pObject->m_NextSiblingIndex;
      
    pParentObject->m_ChildCount--;
    pObject->m_ParentIndex = 0;

    // Note that the sibling indices must not be set to 0 here. 
    // They are still needed if we currently iterate over child objects.
  }
}

void ezWorld::ProcessQueuedMessages(ezObjectMsgQueueType::Enum queueType)
{
  struct MessageComparer
  {
    EZ_FORCE_INLINE bool Less(const ezInternal::WorldData::MessageQueue::Entry& a, const ezInternal::WorldData::MessageQueue::Entry& b) const
    {
      if (a.m_MetaData.m_Due != b.m_MetaData.m_Due)
        return a.m_MetaData.m_Due < b.m_MetaData.m_Due;

      if (a.m_pMessage->GetId() != b.m_pMessage->GetId())
        return a.m_pMessage->GetId() < b.m_pMessage->GetId();

      if (a.m_MetaData.m_ReceiverObject != b.m_MetaData.m_ReceiverObject)
        return a.m_MetaData.m_ReceiverObject < b.m_MetaData.m_ReceiverObject;
          
      return a.m_pMessage->GetHash() < b.m_pMessage->GetHash();
    }
  };

  // regular messages
  {
    ezInternal::WorldData::MessageQueue& queue = m_Data.m_MessageQueues[queueType];
    queue.Sort(MessageComparer());

    for (ezUInt32 i = 0; i < queue.GetCount(); ++i)
    {
      auto& entry = queue[i];

      ezGameObject* pReceiverObject = nullptr;
      if (TryGetObject(entry.m_MetaData.m_ReceiverObject, pReceiverObject))
      {
        pReceiverObject->SendMessage(*entry.m_pMessage, entry.m_MetaData.m_Routing);
      }
    }

    queue.Clear();
  }

  // timed messages
  {
    ezInternal::WorldData::MessageQueue& queue = m_Data.m_TimedMessageQueues[queueType];
    queue.Sort(MessageComparer());

    ezTime now = ezClock::Get(ezGlobalClock_GameLogic)->GetAccumulatedTime();

    while (!queue.IsEmpty())
    {
      auto& entry = queue.Peek();
      if (entry.m_MetaData.m_Due > now)
        break;

      ezGameObject* pReceiverObject = nullptr;
      if (TryGetObject(entry.m_MetaData.m_ReceiverObject, pReceiverObject))
      {
        pReceiverObject->SendMessage(*entry.m_pMessage, entry.m_MetaData.m_Routing);
      }

      EZ_DELETE(&m_Data.m_Allocator, entry.m_pMessage);

      queue.Dequeue();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ezResult ezWorld::RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  EZ_ASSERT_DEV(desc.m_Phase == ezComponentManagerBase::UpdateFunctionDesc::Async || desc.m_uiGranularity == 0, "Granularity must be 0 for synchronous update functions");

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];

  if (desc.m_DependsOn.IsEmpty())
  {
    ezInternal::WorldData::RegisteredUpdateFunction newFunction;
    newFunction.m_Function = desc.m_Function;
    newFunction.m_szFunctionName = desc.m_szFunctionName;
    newFunction.m_uiGranularity = desc.m_uiGranularity;

    updateFunctions.PushBack(newFunction);
  }
  else
  {
    EZ_ASSERT_DEV(desc.m_Phase != ezComponentManagerBase::UpdateFunctionDesc::Async, "Asynchronous update functions must not have dependencies");

    if (RegisterUpdateFunctionWithDependency(desc, true) == EZ_FAILURE)
      return EZ_FAILURE;
  }

  // new function was registered successfully, try to insert unresolved functions
  for (ezUInt32 i = m_Data.m_UnresolvedUpdateFunctions.GetCount(); i-- > 0;)
  {
    if (RegisterUpdateFunctionWithDependency(m_Data.m_UnresolvedUpdateFunctions[i], false) == EZ_SUCCESS)
      m_Data.m_UnresolvedUpdateFunctions.RemoveAt(i);
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
      uiInsertionIndex = ezMath::Max(uiInsertionIndex, uiDependencyIndex + 1);
    }
  }

  ezInternal::WorldData::RegisteredUpdateFunction newFunction;
  newFunction.m_Function = desc.m_Function;
  newFunction.m_szFunctionName = desc.m_szFunctionName;
  newFunction.m_uiGranularity = desc.m_uiGranularity;

  updateFunctions.Insert(newFunction, uiInsertionIndex);

  return EZ_SUCCESS;
}

ezResult ezWorld::DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  ezResult result = EZ_FAILURE;

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];

  for (ezUInt32 i = updateFunctions.GetCount(); i-- > 0;)
  {
    if (updateFunctions[i].m_Function == desc.m_Function)
    {
      updateFunctions.RemoveAt(i);
      result = EZ_SUCCESS;
    }
  }

  return result;
}

void ezWorld::DeregisterUpdateFunctions(ezComponentManagerBase* pManager)
{
  CheckForWriteAccess();

  for (ezUInt32 phase = ezComponentManagerBase::UpdateFunctionDesc::PreAsync; phase < ezComponentManagerBase::UpdateFunctionDesc::PHASE_COUNT; ++phase)
  {
    ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[phase];

    for (ezUInt32 i = updateFunctions.GetCount(); i-- > 0;)
    {
      if (updateFunctions[i].m_Function.GetInstance() == pManager)
      {
        updateFunctions.RemoveAt(i);
      }
    }
  }
}

void ezWorld::UpdateFromThread()
{
  EZ_LOCK(GetWriteMarker());

  Update();
}

void ezWorld::UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions)
{
  for (ezUInt32 i = 0; i < updateFunctions.GetCount(); ++i)
  {
    updateFunctions[i].m_Function(0, ezInvalidIndex);
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
        pTask = EZ_NEW(&m_Data.m_Allocator, ezInternal::WorldData::UpdateTask);
        m_Data.m_UpdateTasks.PushBack(pTask);
      }
            
      pTask->SetTaskName(updateFunction.m_szFunctionName);
      pTask->m_Function = updateFunction.m_Function;
      pTask->m_uiStartIndex = uiStartIndex;
      pTask->m_uiCount = (uiStartIndex + uiGranularity < uiTotalCount) ? uiGranularity : ezInvalidIndex;
      ezTaskSystem::AddTaskToGroup(taskGroupId, pTask);

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
    ezComponent* pMovedComponent = nullptr;
    pManager->DeleteDeadComponent(entry, pMovedComponent);
    
    // another component has been moved to the deleted component location
    if (entry.m_Ptr != pMovedComponent)
    {
      if (ezGameObject* pOwner = entry.m_Ptr->GetOwner())
      {
        pOwner->FixComponentPointer(pMovedComponent, entry.m_Ptr);
      }
    }    
  }

  m_Data.m_DeadComponents.Clear();
}

void ezWorld::PatchHierarchyData(ezGameObject* pObject)
{
  ezGameObject* pParent = pObject->GetParent();

  ezUInt32 uiNewHierarchyLevel = pParent != nullptr ? pParent->m_uiHierarchyLevel + 1 : 0;

  if (uiNewHierarchyLevel != pObject->m_uiHierarchyLevel)
  {
    ezGameObject::TransformationData* pNewTransformationData = nullptr;
    ezUInt32 uiTransformationDataIndex = m_Data.CreateTransformationData(pObject->m_Flags, uiNewHierarchyLevel, pNewTransformationData);

    pNewTransformationData->m_pObject = pObject;
    pNewTransformationData->m_localPosition = pObject->m_pTransformationData->m_localPosition;
    pNewTransformationData->m_localRotation = pObject->m_pTransformationData->m_localRotation;
    pNewTransformationData->m_localScaling = pObject->m_pTransformationData->m_localScaling;
    pNewTransformationData->m_globalTransform = pObject->m_pTransformationData->m_globalTransform;
    pNewTransformationData->m_velocity = pObject->m_pTransformationData->m_velocity;
    pNewTransformationData->m_localBounds = pObject->m_pTransformationData->m_localBounds;
    pNewTransformationData->m_globalBounds = pObject->m_pTransformationData->m_globalBounds;

    m_Data.DeleteTransformationData(pObject->m_Flags, pObject->m_uiHierarchyLevel,
      pObject->m_uiTransformationDataIndex);

    pObject->m_uiHierarchyLevel = uiNewHierarchyLevel;
    pObject->m_uiTransformationDataIndex = uiTransformationDataIndex;
    pObject->m_pTransformationData = pNewTransformationData;
  }

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;
  pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    PatchHierarchyData(it);
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_World);

