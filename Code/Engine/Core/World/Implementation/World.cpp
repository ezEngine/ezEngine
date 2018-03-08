#include <PCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>

ezStaticArray<ezWorld*, 64> ezWorld::s_Worlds;

ezWorld::ezWorld(ezWorldDesc& desc)
  : m_UpdateTask("", ezMakeDelegate(&ezWorld::UpdateFromThread, this))
  , m_Data(desc)
{
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  ezStringBuilder sb = desc.m_sName.GetString();
  sb.Append(".Update");
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
  // allow reading and writing during destruction
  m_Data.m_WriteThreadID = ezThreadUtils::GetCurrentThreadID();
  m_Data.m_iReadCounter.Increment();

  // set all objects to inactive so components and children know that they shouldn't access the objects anymore.
  for (auto it = m_Data.m_ObjectStorage.GetIterator(); it.IsValid(); it.Next())
  {
    it->m_Flags.Remove(ezObjectFlags::Active);
  }

  // deinitialize all modules before we invalidate the world. Components can still access the world during deinitialization.
  for (ezWorldModule* pModule : m_Data.m_Modules)
  {
    if (pModule != nullptr)
    {
      pModule->DeinitializeInternal();
    }
  }

  // now delete all modules
  for (ezWorldModule* pModule : m_Data.m_Modules)
  {
    if (pModule != nullptr)
    {
      EZ_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
  m_Data.m_Modules.Clear();

  s_Worlds[m_uiIndex] = nullptr;
  m_uiIndex = ezInvalidIndex;
}


void ezWorld::Clear()
{
  CheckForWriteAccess();

  for (auto it = GetObjects(); it.IsValid(); ++it)
  {
    DeleteObjectNow(it->GetHandle());
  }
}

ezGameObjectHandle ezWorld::CreateObject(const ezGameObjectDesc& desc, ezGameObject*& out_pObject)
{
  CheckForWriteAccess();

  ezGameObject* pParentObject = nullptr;
  ezGameObject::TransformationData* pParentData = nullptr;
  ezUInt32 uiParentIndex = 0;
  ezUInt32 uiHierarchyLevel = 0;
  bool bDynamic = desc.m_bDynamic;

  if (TryGetObject(desc.m_hParent, pParentObject))
  {
    pParentData = pParentObject->m_pTransformationData;
    uiParentIndex = desc.m_hParent.m_InternalId.m_InstanceIndex;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1; // if there is a parent hierarchy level is parent level + 1
    EZ_ASSERT_DEV(uiHierarchyLevel < (1 << 12), "Max hierarchy level reached");
    bDynamic |= pParentObject->IsDynamic();
  }

  // get storage for the transformation data
  ezGameObject::TransformationData* pTransformationData = m_Data.CreateTransformationData(bDynamic, uiHierarchyLevel);

  // get storage for the object itself
  ezGameObject* pNewObject = m_Data.m_ObjectStorage.Create();

  // insert the new object into the id mapping table
  ezGameObjectId newId = m_Data.m_Objects.Insert(pNewObject);
  newId.m_WorldIndex = m_uiIndex;

  // fill out some data
  pNewObject->m_InternalId = newId;
  pNewObject->m_Flags = ezObjectFlags::None;
  pNewObject->m_Flags.AddOrRemove(ezObjectFlags::Dynamic, bDynamic);
  pNewObject->m_Flags.AddOrRemove(ezObjectFlags::Active, desc.m_bActive);
  pNewObject->m_sName = desc.m_sName;
  pNewObject->m_pWorld = this;
  pNewObject->m_ParentIndex = uiParentIndex;
  pNewObject->m_Tags = desc.m_Tags;
  pNewObject->m_uiTeamID = desc.m_uiTeamID;

  pNewObject->m_uiHierarchyLevel = uiHierarchyLevel;

  // fill out the transformation data
  pTransformationData->m_pObject = pNewObject;
  pTransformationData->m_pParentData = pParentData;
  pTransformationData->m_localPosition = ezSimdConversion::ToVec3(desc.m_LocalPosition);
  pTransformationData->m_localRotation = ezSimdConversion::ToQuat(desc.m_LocalRotation);
  pTransformationData->m_localScaling = ezSimdConversion::ToVec4(desc.m_LocalScaling.GetAsVec4(desc.m_LocalUniformScaling));
  pTransformationData->m_globalTransform.SetIdentity();
  pTransformationData->m_velocity.SetZero();
  pTransformationData->m_localBounds.SetInvalid();
  pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(ezSimdFloat::Zero());
  pTransformationData->m_globalBounds = pTransformationData->m_localBounds;
  pTransformationData->m_hSpatialData.Invalidate();

  if (pParentData != nullptr)
  {
    pTransformationData->UpdateGlobalTransformWithParent();
  }
  else
  {
    pTransformationData->UpdateGlobalTransform();
  }

  pTransformationData->m_lastGlobalPosition = pTransformationData->m_globalTransform.m_Position;

  // link the transformation data to the game object
  pNewObject->m_pTransformationData = pTransformationData;

  // fix links
  LinkToParent(pNewObject);

  out_pObject = pNewObject;
  return ezGameObjectHandle(newId);
}

void ezWorld::DeleteObjectNow(const ezGameObjectHandle& object)
{
  CheckForWriteAccess();

  ezGameObject* pObject = nullptr;
  if (!m_Data.m_Objects.TryGetValue(object, pObject))
    return;

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
    pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle());
  }
  EZ_ASSERT_DEV(pObject->m_Components.GetCount() == 0, "Components should already be removed");

  // fix parent and siblings
  UnlinkFromParent(pObject);

  // remove from global key tables
  SetObjectGlobalKey(pObject, ezHashedString());

  // invalidate and remove from id table
  pObject->m_InternalId.Invalidate();
  m_Data.m_DeadObjects.PushBack(pObject);
  EZ_VERIFY(m_Data.m_Objects.Remove(object), "Implementation error.");
}

void ezWorld::DeleteObjectDelayed(const ezGameObjectHandle& hObject)
{
  ezMsgDeleteGameObject msg;
  PostMessage(hObject, msg, ezObjectMsgQueueType::NextFrame);
}

void ezWorld::PostMessage(const ezGameObjectHandle& receiverObject, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay, bool bRecursive) const
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObject = receiverObject.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent = false;
  metaData.m_uiRecursive = bRecursive;

  if (delay.GetSeconds() > 0.0)
  {
    ezMessage* pMsgCopy = msg.Clone(&m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    ezMessage* pMsgCopy = msg.Clone(m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void ezWorld::PostMessage(const ezComponentHandle& receiverComponent, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay) const
{
  // This method is allowed to be called from multiple threads.

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverComponent = *reinterpret_cast<const ezUInt64*>(&receiverComponent.m_InternalId);
  EZ_ASSERT_DEBUG((metaData.m_uiReceiverData >> 62) == 0, "Upper 2 bits in component id must not be set");

  metaData.m_uiReceiverIsComponent = true;
  metaData.m_uiRecursive = false;

  if (delay.GetSeconds() > 0.0)
  {
    ezMessage* pMsgCopy = msg.Clone(&m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    ezMessage* pMsgCopy = msg.Clone(m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void ezWorld::Update()
{
  CheckForWriteAccess();

  EZ_LOG_BLOCK(m_Data.m_sName.GetData());

  m_Data.m_Clock.SetPaused(!m_Data.m_bSimulateWorld);
  m_Data.m_Clock.Update();

  // initialize phase
  {
    EZ_PROFILE("Initialize Phase");
    ProcessComponentsToInitialize();
    ProcessUpdateFunctionsToRegister();

    ProcessQueuedMessages(ezObjectMsgQueueType::AfterInitialized);
  }

  // pre-async phase
  {
    EZ_PROFILE("Pre-Async Phase");
    ProcessQueuedMessages(ezObjectMsgQueueType::NextFrame);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PreAsync]);
  }

  // async phase
  {
    // remove write marker but keep the read marker. Thus no one can mark the world for writing now. Only reading is allowed in async phase.
    m_Data.m_WriteThreadID = (ezThreadID)0;

    EZ_PROFILE("Async Phase");
    UpdateAsynchronous();

    // restore write marker
    m_Data.m_WriteThreadID = ezThreadUtils::GetCurrentThreadID();
  }

  // post-async phase
  {
    EZ_PROFILE("Post-Async Phase");
    ProcessQueuedMessages(ezObjectMsgQueueType::PostAsync);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PostAsync]);
  }

  // delete dead objects and update the object hierarchy
  {
    EZ_PROFILE("Delete Dead Objects");
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

    EZ_PROFILE("Update Transforms");
    m_Data.UpdateGlobalTransforms(fInvDelta);
  }

  // post-transform phase
  {
    EZ_PROFILE("Post-Transform Phase");
    ProcessQueuedMessages(ezObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  {
    EZ_PROFILE("Initialize Phase 2");
    ProcessComponentsToInitialize();

    ProcessQueuedMessages(ezObjectMsgQueueType::AfterInitialized);
  }

  // Swap our double buffered stack allocator
  m_Data.m_StackAllocator.Swap();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ezWorldModule* ezWorld::GetOrCreateModule(const ezRTTI* pRtti)
{
  CheckForWriteAccess();

  const ezUInt16 uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId == 0xFFFF)
  {
    return nullptr;
  }

  if (uiTypeId >= m_Data.m_Modules.GetCount())
  {
    m_Data.m_Modules.SetCount(uiTypeId + 1);
  }

  ezWorldModule* pModule = m_Data.m_Modules[uiTypeId];
  if (pModule == nullptr)
  {
    pModule = ezWorldModuleFactory::GetInstance()->CreateWorldModule(uiTypeId, this);
    pModule->InitializeInternal();

    m_Data.m_Modules[uiTypeId] = pModule;
  }

  return pModule;
}

void ezWorld::DeleteModule(const ezRTTI* pRtti)
{
  CheckForWriteAccess();

  const ezUInt16 uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (ezWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      pModule->DeinitializeInternal();
      DeregisterUpdateFunctions(pModule);
      EZ_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

ezWorldModule* ezWorld::GetModule(const ezRTTI* pRtti)
{
  CheckForWriteAccess();

  const ezUInt16 uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

const ezWorldModule* ezWorld::GetModule(const ezRTTI* pRtti) const
{
  CheckForReadAccess();

  const ezUInt16 uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

void ezWorld::SetParent(ezGameObject* pObject, ezGameObject* pNewParent, ezGameObject::TransformPreservation preserve)
{
  EZ_ASSERT_DEV(pObject != pNewParent, "Object can't be its own parent!");
  EZ_ASSERT_DEV(pNewParent == nullptr || pObject->IsDynamic() || pNewParent->IsStatic(), "Can't attach a static object to a dynamic parent!");
  CheckForWriteAccess();

  if (GetObjectUnchecked(pObject->m_ParentIndex) == pNewParent)
    return;

  UnlinkFromParent(pObject);
  // UnlinkFromParent does not clear these as they are still needed in DeleteObjectNow to allow deletes while iterating.
  pObject->m_NextSiblingIndex = 0;
  pObject->m_PrevSiblingIndex = 0;
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
  EZ_ASSERT_DEBUG(pObject->m_NextSiblingIndex == 0 && pObject->m_PrevSiblingIndex == 0, "Object is either still linked to another parent or data was not cleared.");
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
    ezLog::Error("Can't set global key to '{0}' because an object with this global key already exists. Global keys have to be unique.", sGlobalKey);
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

  const ezHashedString* pGlobalKey;
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
    ezComponentHandle hComponent(*reinterpret_cast<ezComponentId*>(&uiReceiverComponent));

    ezComponent* pReceiverComponent = nullptr;
    if (TryGetComponent(hComponent, pReceiverComponent))
    {
      pReceiverComponent->SendMessage(*entry.m_pMessage);
    }
    else
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        ezLog::Warning("ezWorld::ProcessQueuedMessage: Receiver ezComponent for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
  else
  {
    ezGameObjectHandle hObject(ezGameObjectId(entry.m_MetaData.m_uiReceiverObject));

    ezGameObject* pReceiverObject = nullptr;
    if (TryGetObject(hObject, pReceiverObject))
    {
      if (entry.m_MetaData.m_uiRecursive)
      {
        pReceiverObject->SendMessageRecursive(*entry.m_pMessage);
      }
      else
      {
        pReceiverObject->SendMessage(*entry.m_pMessage);
      }
    }
    else
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        ezLog::Warning("ezWorld::ProcessQueuedMessage: Receiver ezGameObject for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
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

      if (a.m_MetaData.m_uiReceiverComponent != b.m_MetaData.m_uiReceiverComponent)
        return a.m_MetaData.m_uiReceiverComponent < b.m_MetaData.m_uiReceiverComponent;

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

void ezWorld::RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  EZ_ASSERT_DEV(desc.m_Phase == ezComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_uiGranularity == 0, "Granularity must be 0 for synchronous update functions");
  EZ_ASSERT_DEV(desc.m_Phase != ezComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_DependsOn.GetCount() == 0, "Asynchronous update functions must not have dependencies");

  m_Data.m_UpdateFunctionsToRegister.PushBack(desc);
}

void ezWorld::DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];

  for (ezUInt32 i = updateFunctions.GetCount(); i-- > 0;)
  {
    if (updateFunctions[i].m_Function == desc.m_Function)
    {
      updateFunctions.RemoveAt(i);
    }
  }
}

void ezWorld::DeregisterUpdateFunctions(ezWorldModule* pModule)
{
  CheckForWriteAccess();

  for (ezUInt32 phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync; phase < ezWorldModule::UpdateFunctionDesc::Phase::COUNT; ++phase)
  {
    ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[phase];

    for (ezUInt32 i = updateFunctions.GetCount(); i-- > 0;)
    {
      if (updateFunctions[i].m_Function.GetClassInstance() == pModule)
      {
        updateFunctions.RemoveAt(i);
      }
    }
  }
}

void ezWorld::AddComponentToInitialize(ezComponentHandle hComponent)
{
  m_Data.m_ComponentsToInitialize.PushBack(hComponent);
}

void ezWorld::UpdateFromThread()
{
  EZ_LOCK(GetWriteMarker());

  Update();
}

void ezWorld::UpdateSynchronous(const ezArrayPtr<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions)
{
  ezWorldModule::UpdateContext context;
  context.m_uiFirstComponentIndex = 0;
  context.m_uiComponentCount = ezInvalidIndex;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    {
      EZ_PROFILE(updateFunction.m_sFunctionName);
      updateFunction.m_Function(context);
    }
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

      pTask->SetTaskName(updateFunction.m_sFunctionName);
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

void ezWorld::ProcessComponentsToInitialize()
{
  CheckForWriteAccess();

  EZ_PROFILE("Initialize Components");

  // Can't use foreach here because the array might be resized during iteration.
  for (ezUInt32 i = 0; i < m_Data.m_ComponentsToInitialize.GetCount(); ++i)
  {
    ezComponentHandle hComponent = m_Data.m_ComponentsToInitialize[i];

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

    pComponent->EnsureInitialized();

    if (pComponent->IsActive())
    {
      pComponent->OnActivated();

      m_Data.m_ComponentsToStartSimulation.PushBack(hComponent);
    }
  }

  m_Data.m_ComponentsToInitialize.Clear();

  if (m_Data.m_bSimulateWorld)
  {
    // Can't use foreach here because the array might be resized during iteration.
    for (ezUInt32 i = 0; i < m_Data.m_ModulesToStartSimulation.GetCount(); ++i)
    {
      m_Data.m_ModulesToStartSimulation[i]->OnSimulationStarted();
    }

    m_Data.m_ModulesToStartSimulation.Clear();

    // Can't use foreach here because the array might be resized during iteration.
    for (ezUInt32 i = 0; i < m_Data.m_ComponentsToStartSimulation.GetCount(); ++i)
    {
      ezComponentHandle hComponent = m_Data.m_ComponentsToStartSimulation[i];

      ezComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent)) // if it is in the editor, the component might have been added and already deleted, without ever running the simulation
        continue;

      if (pComponent->IsActiveAndInitialized())
      {
        pComponent->EnsureSimulationStarted();
      }
    }

    m_Data.m_ComponentsToStartSimulation.Clear();
  }
}

void ezWorld::ProcessUpdateFunctionsToRegister()
{
  CheckForWriteAccess();

  if (m_Data.m_UpdateFunctionsToRegister.IsEmpty())
    return;

  EZ_PROFILE("Register update functions");

  while (!m_Data.m_UpdateFunctionsToRegister.IsEmpty())
  {
    const ezUInt32 uiNumFunctionsToRegister = m_Data.m_UpdateFunctionsToRegister.GetCount();

    for (ezUInt32 i = uiNumFunctionsToRegister; i-- > 0; )
    {
      if (RegisterUpdateFunctionInternal(m_Data.m_UpdateFunctionsToRegister[i]).Succeeded())
      {
        m_Data.m_UpdateFunctionsToRegister.RemoveAt(i);
      }
    }

    EZ_ASSERT_DEV(m_Data.m_UpdateFunctionsToRegister.GetCount() < uiNumFunctionsToRegister, "No functions have been registered because the dependencies could not be found.");
  }
}

ezResult ezWorld::RegisterUpdateFunctionInternal(const ezWorldModule::UpdateFunctionDesc& desc)
{
  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase];
  ezUInt32 uiInsertionIndex = 0;

  for (ezUInt32 i = 0; i < desc.m_DependsOn.GetCount(); ++i)
  {
    ezUInt32 uiDependencyIndex = ezInvalidIndex;

    for (ezUInt32 j = 0; j < updateFunctions.GetCount(); ++j)
    {
      if (updateFunctions[j].m_sFunctionName == desc.m_DependsOn[i])
      {
        uiDependencyIndex = j;
        break;
      }
    }

    if (uiDependencyIndex == ezInvalidIndex) // dependency not found
    {
      return EZ_FAILURE;
    }
    else
    {
      uiInsertionIndex = ezMath::Max(uiInsertionIndex, uiDependencyIndex + 1);
    }
  }

  ezInternal::WorldData::RegisteredUpdateFunction newFunction;
  newFunction.FillFromDesc(desc);

  while (uiInsertionIndex < updateFunctions.GetCount())
  {
    const auto& existingFunction = updateFunctions[uiInsertionIndex];
    if (newFunction < existingFunction)
    {
      break;
    }

    ++uiInsertionIndex;
  }

  updateFunctions.Insert(newFunction, uiInsertionIndex);

  return EZ_SUCCESS;
}

void ezWorld::DeleteDeadObjects()
{
  while (!m_Data.m_DeadObjects.IsEmpty())
  {
    ezGameObject* pObject = m_Data.m_DeadObjects.PeekBack();
    m_Data.m_DeadObjects.PopBack();

    if (!pObject->m_pTransformationData->m_hSpatialData.IsInvalidated())
    {
      m_Data.m_pSpatialSystem->DeleteSpatialData(pObject->m_pTransformationData->m_hSpatialData);
    }

    m_Data.DeleteTransformationData(pObject->IsDynamic(), pObject->m_uiHierarchyLevel, pObject->m_pTransformationData);

    ezGameObject* pMovedObject = nullptr;
    m_Data.m_ObjectStorage.Delete(pObject, pMovedObject);

    if (pObject != pMovedObject)
    {
      // patch the id table: the last element in the storage has been moved to deleted object's location,
      // thus the pointer now points to another object
      ezGameObjectId id = pObject->m_InternalId;
      if (id.m_InstanceIndex != ezGameObjectId::INVALID_INSTANCE_INDEX)
        m_Data.m_Objects[id] = pObject;

      // the moved object might be deleted as well so we need to patch the dead objects list
      for (ezUInt32 i = 0; i < m_Data.m_DeadObjects.GetCount(); ++i)
      {
        if (m_Data.m_DeadObjects[i] == pMovedObject)
        {
          m_Data.m_DeadObjects[i] = pObject;
          break;
        }
      }
    }
  }
}

void ezWorld::DeleteDeadComponents()
{
  while (!m_Data.m_DeadComponents.IsEmpty())
  {
    ezComponent* pComponent = m_Data.m_DeadComponents.PeekBack();
    m_Data.m_DeadComponents.PopBack();

    ezComponentManagerBase* pManager = pComponent->GetOwningManager();
    ezComponent* pMovedComponent = nullptr;
    pManager->DeleteComponentStorage(pComponent, pMovedComponent);

    // another component has been moved to the deleted component location
    if (pComponent != pMovedComponent)
    {
      pManager->PatchIdTable(pComponent);

      if (ezGameObject* pOwner = pComponent->GetOwner())
      {
        pOwner->FixComponentPointer(pMovedComponent, pComponent);
      }

      // the moved component might be deleted as well so we need to patch the dead components list
      for (ezUInt32 i = 0; i < m_Data.m_DeadComponents.GetCount(); ++i)
      {
        if (m_Data.m_DeadComponents[i] == pMovedComponent)
        {
          m_Data.m_DeadComponents[i] = pComponent;
          break;
        }
      }
    }
  }
}

void ezWorld::PatchHierarchyData(ezGameObject* pObject, ezGameObject::TransformPreservation preserve)
{
  ezGameObject* pParent = pObject->GetParent();

  RecreateHierarchyData(pObject, pObject->IsDynamic());

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

void ezWorld::RecreateHierarchyData(ezGameObject* pObject, bool bWasDynamic)
{
  ezGameObject* pParent = pObject->GetParent();

  const ezUInt32 uiNewHierarchyLevel = pParent != nullptr ? pParent->m_uiHierarchyLevel + 1 : 0;
  const ezUInt32 uiOldHierarchyLevel = pObject->m_uiHierarchyLevel;

  const bool bIsDynamic = pObject->IsDynamic();

  if (uiNewHierarchyLevel != uiOldHierarchyLevel || bIsDynamic != bWasDynamic)
  {
    ezGameObject::TransformationData* pOldTransformationData = pObject->m_pTransformationData;

    ezGameObject::TransformationData* pNewTransformationData = m_Data.CreateTransformationData(bIsDynamic, uiNewHierarchyLevel);
    ezMemoryUtils::Copy(pNewTransformationData, pOldTransformationData, 1);

    pObject->m_uiHierarchyLevel = uiNewHierarchyLevel;
    pObject->m_pTransformationData = pNewTransformationData;

    // fix parent transform data for children as well
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      ezGameObject::TransformationData* pTransformData = it->m_pTransformationData;
      pTransformData->m_pParentData = pNewTransformationData;
    }

    m_Data.DeleteTransformationData(bWasDynamic, uiOldHierarchyLevel, pOldTransformationData);
  }
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_World);

