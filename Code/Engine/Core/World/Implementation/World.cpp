#include <Core/PCH.h>


#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Core/Messages/DeleteObjectMessage.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezWorldModule, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE

static ezProfilingId s_PreAsyncProfilingID = ezProfilingSystem::CreateId("Pre-Async Phase");
static ezProfilingId s_AsyncProfilingID = ezProfilingSystem::CreateId("Async Phase");
static ezProfilingId s_PostAsyncProfilingID = ezProfilingSystem::CreateId("Post-Async Phase");
static ezProfilingId s_UpdateTransformsProfilingID = ezProfilingSystem::CreateId("Update Transforms");
static ezProfilingId s_PostTransformProfilingID = ezProfilingSystem::CreateId("Post-Transform Phase");
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

  pNewObject->m_uiHierarchyLevel = uiHierarchyLevel;
  pNewObject->m_uiTransformationDataIndex = uiTransformationDataIndex;

  // fill out the transformation data
  pTransformationData->m_pObject = pNewObject;
  pTransformationData->m_pParentData = pParentData;
  pTransformationData->m_localPosition = desc.m_LocalPosition.GetAsPositionVec4();
  pTransformationData->m_localRotation = desc.m_LocalRotation;
  pTransformationData->m_localScaling = desc.m_LocalScaling.GetAsDirectionVec4();
  pTransformationData->m_localScaling.w = desc.m_LocalUniformScaling;
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

  // fix links
  LinkToParent(pNewObject);

  out_pObject = pNewObject;
  return newId;
}

void ezWorld::DeleteObjectNow(const ezGameObjectHandle& object)
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
    DeleteObjectNow(it->GetHandle());
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

  // remove from global key tables
  SetObjectGlobalKey(pObject, ezHashedString());

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

ezComponentManagerBase* ezWorld::GetOrCreateComponentManager(const ezRTTI* pRtti)
{
  if (pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract))
    return nullptr;

  CheckForWriteAccess();

  const ezUInt16 uiTypeId = ezComponentManagerFactory::GetInstance()->GetTypeId(pRtti);
  EZ_ASSERT_DEV(uiTypeId != -1, "Invalid type id");

  if (uiTypeId >= m_Data.m_ComponentManagers.GetCount())
  {
    m_Data.m_ComponentManagers.SetCount(uiTypeId + 1);
  }

  ezComponentManagerBase* pManager = m_Data.m_ComponentManagers[uiTypeId];
  if (pManager == nullptr)
  {
    pManager = ezComponentManagerFactory::GetInstance()->CreateComponentManager(uiTypeId, this);
    pManager->Initialize();

    m_Data.m_ComponentManagers[uiTypeId] = pManager;
  }

  return pManager;
}

const ezComponentManagerBase* ezWorld::GetComponentManager(const ezRTTI* pRtti) const
{
  CheckForReadAccess();

  const ezUInt16 uiTypeId = ezComponentManagerFactory::GetInstance()->GetTypeId(pRtti);
  EZ_ASSERT_DEV(uiTypeId != -1, "Invalid type id");

  if (uiTypeId < m_Data.m_ComponentManagers.GetCount())
  {
    return m_Data.m_ComponentManagers[uiTypeId];
  }

  return nullptr;
}

void ezWorld::PostMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg,
                          ezObjectMsgQueueType::Enum queueType, ezObjectMsgRouting::Enum routing)
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObject = receiverObject.m_InternalId.m_Data;
  metaData.m_Routing = routing;

  ezMessage* pMsgCopy = msg.Clone(ezFrameAllocator::GetCurrentAllocator());
  m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
}

void ezWorld::PostMessage(const ezGameObjectHandle& receiverObject, ezMessage& msg,
                          ezObjectMsgQueueType::Enum queueType, ezTime delay, ezObjectMsgRouting::Enum routing)
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObject = receiverObject.m_InternalId.m_Data;
  metaData.m_Routing = routing;
  metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;

  ezMessage* pMsgCopy = msg.Clone(&m_Data.m_Allocator);
  m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
}

void ezWorld::PostMessage(const ezComponentHandle& receiverComponent, ezMessage& msg, ezObjectMsgQueueType::Enum queueType)
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverComponent = *reinterpret_cast<const ezUInt64*>(&receiverComponent.m_InternalId);
  metaData.m_uiReceiverIsComponent = 1;
  
  ezMessage* pMsgCopy = msg.Clone(ezFrameAllocator::GetCurrentAllocator());
  m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
}

void ezWorld::PostMessage(const ezComponentHandle& receiverComponent, ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay)
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverComponent = *reinterpret_cast<const ezUInt64*>(&receiverComponent.m_InternalId);
  metaData.m_uiReceiverIsComponent = 1;
  metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;

  ezMessage* pMsgCopy = msg.Clone(&m_Data.m_Allocator);
  m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
}


void ezWorld::Update()
{
  CheckForWriteAccess();
  EZ_ASSERT_DEV(m_Data.m_UnresolvedUpdateFunctions.IsEmpty(), "There are update functions with unresolved dependencies.");

  EZ_LOG_BLOCK(m_Data.m_sName.GetData());
  EZ_PROFILE(m_UpdateProfilingID);

  m_Data.m_Clock.SetPaused(!m_Data.m_bSimulateWorld);
  m_Data.m_Clock.Update();

  ProcessComponentsToInitialize();

  // pre-async phase
  {
    EZ_PROFILE(s_PreAsyncProfilingID);
    ProcessQueuedMessages(ezObjectMsgQueueType::NextFrame);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PreAsync]);
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
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PostAsync]);
  }

  // delete dead objects and update the object hierarchy
  {
    EZ_PROFILE(s_DeleteDeadObjectsProfilingID);
    DeleteDeadObjects();
    DeleteDeadComponents();
  }

  // update transforms
  {
    float fInvDelta = 0.0f;

    // when the clock is paused just use zero
    const float fDelta = (float)m_Data.m_Clock.GetTimeDiff().GetSeconds();
    if (fDelta > 0.0f)
      fInvDelta = 1.0f / fDelta;

    EZ_PROFILE(s_UpdateTransformsProfilingID);
    m_Data.UpdateGlobalTransforms(fInvDelta);
  }

  // post-transform phase
  {
    EZ_PROFILE(s_PostTransformProfilingID);
    ProcessQueuedMessages(ezObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  ProcessComponentsToInitialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ezWorld::SetParent(ezGameObject* pObject, ezGameObject* pNewParent, ezGameObject::TransformPreservation preserve)
{
  EZ_ASSERT_DEV(pObject != pNewParent, "Object can't be its own parent!");
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

  PatchHierarchyData(pObject, preserve);
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

    pObject->m_pTransformationData->m_pParentData = pParentObject->m_pTransformationData;
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
    pObject->m_pTransformationData->m_pParentData = nullptr;

    // Note that the sibling indices must not be set to 0 here. 
    // They are still needed if we currently iterate over child objects.
  }
}

void ezWorld::SetObjectGlobalKey(ezGameObject* pObject, const ezHashedString& sGlobalKey)
{
  if (m_Data.m_GlobalKeyToIdTable.Contains(sGlobalKey.GetHash()))
  {
    ezLog::Error("Can't set global key to '%s' because an object with this global key already exists. Global key have to be unique.", sGlobalKey.GetData());
    return;
  }

  const ezUInt32 uiId = pObject->m_InternalId.m_Data;

  // Remove existing entry first.
  ezHashedString* pOldGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pOldGlobalKey))
  {
    if (sGlobalKey == *pOldGlobalKey)
    {
      return;
    }

    EZ_VERIFY(m_Data.m_GlobalKeyToIdTable.Remove(pOldGlobalKey->GetHash()), "Implementation error.");
    EZ_VERIFY(m_Data.m_IdToGlobalKeyTable.Remove(uiId), "Implementation error.");
  }

  // Insert new one if key is valid.
  if (!sGlobalKey.IsEmpty())
  {
    m_Data.m_GlobalKeyToIdTable.Insert(sGlobalKey.GetHash(), pObject->m_InternalId);
    m_Data.m_IdToGlobalKeyTable.Insert(uiId, sGlobalKey);
  }
}


const char* ezWorld::GetObjectGlobalKey(const ezGameObject* pObject) const
{
  const ezUInt32 uiId = pObject->m_InternalId.m_Data;

  ezHashedString* pGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pGlobalKey))
  {
    return pGlobalKey->GetData();
  }

  return "";
}

void ezWorld::ProcessQueuedMessage(const ezInternal::WorldData::MessageQueue::Entry& entry)
{
  if (entry.m_MetaData.m_uiReceiverIsComponent)
  {
    ezUInt64 uiReceiverComponent = entry.m_MetaData.m_uiReceiverComponent;
    ezComponentId componentId = *reinterpret_cast<ezComponentId*>(&uiReceiverComponent);

    ezComponent* pReceiverComponent = nullptr;
    if (TryGetComponent(componentId, pReceiverComponent))
    {
      pReceiverComponent->SendMessage(*entry.m_pMessage);
    }
  }
  else
  {
    ezGameObjectId objectId = entry.m_MetaData.m_uiReceiverObject;

    ezGameObject* pReceiverObject = nullptr;
    if (TryGetObject(objectId, pReceiverObject))
    {
      pReceiverObject->SendMessage(*entry.m_pMessage, entry.m_MetaData.m_Routing);
    }
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

      const ezInt32 iKeyA = a.m_pMessage->GetSortingKey();
      const ezInt32 iKeyB = b.m_pMessage->GetSortingKey();
      if (iKeyA != iKeyB)
        return iKeyA < iKeyB;

      if (a.m_pMessage->GetId() != b.m_pMessage->GetId())
        return a.m_pMessage->GetId() < b.m_pMessage->GetId();

      if (a.m_MetaData.m_uiReceiverAndRouting != b.m_MetaData.m_uiReceiverAndRouting)
        return a.m_MetaData.m_uiReceiverAndRouting < b.m_MetaData.m_uiReceiverAndRouting;

      return a.m_pMessage->GetHash() < b.m_pMessage->GetHash();
    }
  };

  // regular messages
  {
    ezInternal::WorldData::MessageQueue& queue = m_Data.m_MessageQueues[queueType];
    queue.Sort(MessageComparer());

    for (ezUInt32 i = 0; i < queue.GetCount(); ++i)
    {
      ProcessQueuedMessage(queue[i]);      

      // no need to deallocate these messages, they are allocated through a frame allocator
    }

    queue.Clear();
  }

  // timed messages
  {
    ezInternal::WorldData::MessageQueue& queue = m_Data.m_TimedMessageQueues[queueType];
    queue.Sort(MessageComparer());

    const ezTime now = m_Data.m_Clock.GetAccumulatedTime();

    while (!queue.IsEmpty())
    {
      auto& entry = queue.Peek();
      if (entry.m_MetaData.m_Due > now)
        break;

      ProcessQueuedMessage(entry);

      EZ_DELETE(&m_Data.m_Allocator, entry.m_pMessage);

      queue.Dequeue();
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ezResult ezWorld::RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  EZ_ASSERT_DEV(desc.m_Phase == ezComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_uiGranularity == 0, "Granularity must be 0 for synchronous update functions");

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];

  if (desc.m_DependsOn.IsEmpty())
  {
    ezInternal::WorldData::RegisteredUpdateFunction& newFunction = updateFunctions.ExpandAndGetRef();
    newFunction.m_Function = desc.m_Function;
    newFunction.m_szFunctionName = desc.m_szFunctionName;
    newFunction.m_bOnlyUpdateWhenSimulating = desc.m_bOnlyUpdateWhenSimulating;
    newFunction.m_uiGranularity = desc.m_uiGranularity;
  }
  else
  {
    EZ_ASSERT_DEV(desc.m_Phase != ezComponentManagerBase::UpdateFunctionDesc::Phase::Async, "Asynchronous update functions must not have dependencies");

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
  newFunction.m_bOnlyUpdateWhenSimulating = desc.m_bOnlyUpdateWhenSimulating;
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

  for (ezUInt32 phase = ezComponentManagerBase::UpdateFunctionDesc::Phase::PreAsync; phase < ezComponentManagerBase::UpdateFunctionDesc::Phase::COUNT; ++phase)
  {
    ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[phase];

    for (ezUInt32 i = updateFunctions.GetCount(); i-- > 0;)
    {
      if (updateFunctions[i].m_Function.GetClassInstance() == pManager)
      {
        updateFunctions.RemoveAt(i);
      }
    }
  }
}

void ezWorld::AddComponentToInitialize(ezComponentHandle hComponent)
{
  m_Data.m_ComponentsToInitialize[m_Data.m_uiCurrentInitializeQueue].PushBack(hComponent);
  m_Data.m_ComponentsToStartSimulation[m_Data.m_uiCurrentSimulationQueue].PushBack(hComponent);
}

void ezWorld::UpdateFromThread()
{
  EZ_LOCK(GetWriteMarker());

  Update();
}


void ezWorld::ProcessComponentsToInitialize()
{
  CheckForWriteAccess();

  while (!m_Data.m_ComponentsToInitialize[0].IsEmpty() || !m_Data.m_ComponentsToInitialize[1].IsEmpty())
  {
    const ezUInt32 uiThisQueue = m_Data.m_uiCurrentInitializeQueue;
    m_Data.m_uiCurrentInitializeQueue = (uiThisQueue + 1) % 2;

    for (ezComponentHandle hComponent : m_Data.m_ComponentsToInitialize[uiThisQueue])
    {
      ezComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent)) // if it is in the editor, the component might have been added and already deleted, without ever running the simulation
        continue;

      // may have been initialized by someone else in the mean time
      if (pComponent->IsInitialized())
        continue;

      // make sure the object's transform is up to date before the component is initialized
      if (pComponent->GetOwner())
      {
        pComponent->GetOwner()->UpdateGlobalTransform();
      }

      pComponent->m_ComponentFlags.Add(ezObjectFlags::Initializing);

      pComponent->Initialize();

      pComponent->m_ComponentFlags.Remove(ezObjectFlags::Initializing);
      pComponent->m_ComponentFlags.Add(ezObjectFlags::Initialized);
    }

    m_Data.m_ComponentsToInitialize[uiThisQueue].Clear();

    if (!m_Data.m_ComponentsToInitialize[m_Data.m_uiCurrentInitializeQueue].IsEmpty())
    {
      // this is here to figure out whether that can actually ever happen, it's not a problem, that's what the ping-pong array is for
      ezLog::Warning("Components were added to initialization queue while initializing components!");
    }
  }

  if (m_Data.m_bSimulateWorld)
  {
    while (!m_Data.m_ComponentsToStartSimulation[0].IsEmpty() || !m_Data.m_ComponentsToStartSimulation[1].IsEmpty())
    {
      const ezUInt32 uiThisQueue = m_Data.m_uiCurrentSimulationQueue;
      m_Data.m_uiCurrentSimulationQueue = (uiThisQueue + 1) % 2;

      for (ezComponentHandle hComponent : m_Data.m_ComponentsToStartSimulation[uiThisQueue])
      {
        ezComponent* pComponent = nullptr;
        if (!TryGetComponent(hComponent, pComponent)) // if it is in the editor, the component might have been added and already deleted, without ever running the simulation
        {
          // in an editor setting this can happen
          // in a runtime setting this should actually not be possible, unless there is a multi-threading bug :-(
          ezLog::Warning("Component in sim queue is invalid");
          continue;
        }

        // may have been called by someone else in the mean time
        if (pComponent->IsSimulationStarted())
          continue;

        pComponent->m_ComponentFlags.Add(ezObjectFlags::SimulationStarting);

        pComponent->OnSimulationStarted();

        pComponent->m_ComponentFlags.Remove(ezObjectFlags::SimulationStarting);
        pComponent->m_ComponentFlags.Add(ezObjectFlags::SimulationStarted);
      }

      m_Data.m_ComponentsToStartSimulation[uiThisQueue].Clear();

      if (!m_Data.m_ComponentsToStartSimulation[m_Data.m_uiCurrentSimulationQueue].IsEmpty())
      {
        // this is here to figure out whether that can actually ever happen, it's not a problem, that's what the ping-pong array is for
        ezLog::Warning("Components were added to simulation start queue while starting components!");
      }
    }
  }
}

void ezWorld::UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions)
{
  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    updateFunction.m_Function(0, ezInvalidIndex);
  }
}

void ezWorld::UpdateAsynchronous()
{
  ezTaskGroupID taskGroupId = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions =
    m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::Async];

  ezUInt32 uiCurrentTaskIndex = 0;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    ezComponentManagerBase* pManager = static_cast<ezComponentManagerBase*>(updateFunction.m_Function.GetClassInstance());

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

void ezWorld::PatchHierarchyData(ezGameObject* pObject, ezGameObject::TransformPreservation preserve)
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

    ezUInt32 m_uiOldHierarchyLevel = pObject->m_uiHierarchyLevel;
    ezUInt32 uiOldTransformationDataIndex = pObject->m_uiTransformationDataIndex;

    pObject->m_uiHierarchyLevel = uiNewHierarchyLevel;
    pObject->m_uiTransformationDataIndex = uiTransformationDataIndex;
    pObject->m_pTransformationData = pNewTransformationData;

    m_Data.DeleteTransformationData(pObject->m_Flags, m_uiOldHierarchyLevel, uiOldTransformationDataIndex);
  }

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;

  if (preserve == ezGameObject::TransformPreservation::PreserveGlobal)
  {
    pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);
  }
  else
  {
    pObject->UpdateGlobalTransform();
  }

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    PatchHierarchyData(it, preserve);
  }
  EZ_ASSERT_DEBUG(pObject->m_pTransformationData != pObject->m_pTransformationData->m_pParentData, "Hierarchy corrupted!");
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_World);

