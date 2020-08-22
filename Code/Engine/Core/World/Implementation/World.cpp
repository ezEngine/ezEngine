#include <CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/EventMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/Stats.h>

ezStaticArray<ezWorld*, ezWorld::GetMaxNumWorlds()> ezWorld::s_Worlds;

static ezGameObjectHandle DefaultGameObjectReferenceResolver(const void* pData, ezComponentHandle hThis, const char* szProperty)
{
  const char* szRef = reinterpret_cast<const char*>(pData);

  if (ezStringUtils::IsNullOrEmpty(szRef))
    return ezGameObjectHandle();

  // this is a convention used by ezPrefabReferenceComponent:
  // a string starting with this means a 'global game object reference', ie a reference that is valid within the current world
  // what follows is an integer that is the internal storage of an ezGameObjectHandle
  // thus parsing the int and casting it to an ezGameObjectHandle gives the desired result
  if (ezStringUtils::StartsWith(szRef, "#!GGOR-"))
  {
    ezInt64 id;
    if (ezConversionUtils::StringToInt64(szRef + 7, id).Succeeded())
    {
      return ezGameObjectHandle(ezGameObjectId(reinterpret_cast<ezUInt64&>(id)));
    }
  }

  return ezGameObjectHandle();
}

ezWorld::ezWorld(ezWorldDesc& desc)
  : m_Data(desc)
{
  m_pUpdateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "", ezMakeDelegate(&ezWorld::UpdateFromThread, this));
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  ezStringBuilder sb = desc.m_sName.GetString();
  sb.Append(".Update");
  m_pUpdateTask->ConfigureTask(sb, ezTaskNesting::Maybe);

  m_uiIndex = ezInvalidIndex;

  // find a free world slot
  const ezUInt32 uiWorldCount = s_Worlds.GetCount();
  for (ezUInt32 i = 0; i < uiWorldCount; i++)
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
    EZ_ASSERT_DEV(m_uiIndex < GetMaxNumWorlds(), "Max world index reached: {}", GetMaxNumWorlds());
    static_assert((GetMaxNumWorlds() - 1) <= ezMath::MaxValue<ezUInt8>()); // World index is stored in ezUInt8 in game objects

    s_Worlds.PushBack(this);
  }

  SetGameObjectReferenceResolver(DefaultGameObjectReferenceResolver);
}

ezWorld::~ezWorld()
{
  // allow reading and writing during destruction
  m_Data.m_WriteThreadID = ezThreadUtils::GetCurrentThreadID();
  m_Data.m_iReadCounter.Increment();

  // deactivate all objects and components before destroying them
  for (auto it = m_Data.m_ObjectStorage.GetIterator(); it.IsValid(); it.Next())
  {
    it->SetActiveFlag(false);
  }

  // deinitialize all modules before we invalidate the world. Components can still access the world during deinitialization.
  for (ezWorldModule* pModule : m_Data.m_Modules)
  {
    if (pModule != nullptr)
    {
      pModule->Deinitialize();
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

  while (GetObjectCount() > 0)
  {
    for (auto it = GetObjects(); it.IsValid(); ++it)
    {
      DeleteObjectNow(it->GetHandle());
    }

    if (GetObjectCount() > 0)
    {
      ezLog::Dev("Remaining objects after ezWorld::Clear: {}", GetObjectCount());
    }
  }

  for (ezWorldModule* pModule : m_Data.m_Modules)
  {
    if (pModule != nullptr)
    {
      pModule->WorldClear();
    }
  }

  // make sure all dead objects and components are cleared right now
  DeleteDeadObjects();
  DeleteDeadComponents();
}

void ezWorld::SetCoordinateSystemProvider(const ezSharedPtr<ezCoordinateSystemProvider>& pProvider)
{
  EZ_ASSERT_DEV(pProvider != nullptr, "Coordinate System Provider must not be null");

  m_Data.m_pCoordinateSystemProvider = pProvider;
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;
}

void ezWorld::SetGameObjectReferenceResolver(const ReferenceResolver& resolver)
{
  m_Data.m_GameObjectReferenceResolver = resolver;
}

const ezWorld::ReferenceResolver& ezWorld::GetGameObjectReferenceResolver() const
{
  return m_Data.m_GameObjectReferenceResolver;
}

ezGameObjectHandle ezWorld::CreateObject(const ezGameObjectDesc& desc, ezGameObject*& out_pObject)
{
  CheckForWriteAccess();

  EZ_ASSERT_DEV(m_Data.m_Objects.GetCount() < GetMaxNumGameObjects(), "Max number of game objects reached: {}", GetMaxNumGameObjects());

  ezGameObject* pParentObject = nullptr;
  ezGameObject::TransformationData* pParentData = nullptr;
  ezUInt32 uiParentIndex = 0;
  ezUInt64 uiHierarchyLevel = 0;
  bool bDynamic = desc.m_bDynamic;

  if (TryGetObject(desc.m_hParent, pParentObject))
  {
    pParentData = pParentObject->m_pTransformationData;
    uiParentIndex = desc.m_hParent.m_InternalId.m_InstanceIndex;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1; // if there is a parent hierarchy level is parent level + 1
    EZ_ASSERT_DEV(uiHierarchyLevel < GetMaxNumHierarchyLevels(), "Max hierarchy level reached: {}", GetMaxNumHierarchyLevels());
    bDynamic |= pParentObject->IsDynamic();
  }

  // get storage for the transformation data
  ezGameObject::TransformationData* pTransformationData = m_Data.CreateTransformationData(bDynamic, static_cast<ezUInt32>(uiHierarchyLevel));

  // get storage for the object itself
  ezGameObject* pNewObject = m_Data.m_ObjectStorage.Create();

  // insert the new object into the id mapping table
  ezGameObjectId newId = m_Data.m_Objects.Insert(pNewObject);
  newId.m_WorldIndex = static_cast<ezUInt8>(m_uiIndex);

  // fill out some data
  pNewObject->m_InternalId = newId;
  pNewObject->m_Flags = ezObjectFlags::None;
  pNewObject->m_Flags.AddOrRemove(ezObjectFlags::Dynamic, bDynamic);
  pNewObject->m_Flags.AddOrRemove(ezObjectFlags::ActiveFlag, desc.m_bActiveFlag);
  pNewObject->m_sName = desc.m_sName;
  pNewObject->m_ParentIndex = uiParentIndex;
  pNewObject->m_Tags = desc.m_Tags;
  pNewObject->m_uiTeamID = desc.m_uiTeamID;

  static_assert((GetMaxNumHierarchyLevels() - 1) <= ezMath::MaxValue<ezUInt16>());
  pNewObject->m_uiHierarchyLevel = static_cast<ezUInt16>(uiHierarchyLevel);

  // fill out the transformation data
  pTransformationData->m_pObject = pNewObject;
  pTransformationData->m_pParentData = pParentData;
  pTransformationData->m_localPosition = ezSimdConversion::ToVec3(desc.m_LocalPosition);
  pTransformationData->m_localRotation = ezSimdConversion::ToQuat(desc.m_LocalRotation);
  pTransformationData->m_localScaling = ezSimdConversion::ToVec4(desc.m_LocalScaling.GetAsVec4(desc.m_LocalUniformScaling));
  pTransformationData->m_globalTransform.SetIdentity();
#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
  pTransformationData->m_velocity.SetZero();
#endif
  pTransformationData->m_localBounds.SetInvalid();
  pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(ezSimdFloat::Zero());
  pTransformationData->m_globalBounds = pTransformationData->m_localBounds;
  pTransformationData->m_hSpatialData.Invalidate();
  pTransformationData->m_uiSpatialDataCategoryBitmask = 0;

  if (pParentData != nullptr)
  {
    pTransformationData->UpdateGlobalTransformWithParent();
  }
  else
  {
    pTransformationData->UpdateGlobalTransform();
  }

#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
  pTransformationData->m_lastGlobalPosition = pTransformationData->m_globalTransform.m_Position;
#endif

  // link the transformation data to the game object
  pNewObject->m_pTransformationData = pTransformationData;

  // fix links
  LinkToParent(pNewObject);

  pNewObject->UpdateActiveState(pParentObject == nullptr ? true : pParentObject->IsActive());

  out_pObject = pNewObject;
  return ezGameObjectHandle(newId);
}

void ezWorld::DeleteObjectNow(const ezGameObjectHandle& hObject)
{
  CheckForWriteAccess();

  ezGameObject* pObject = nullptr;
  if (!m_Data.m_Objects.TryGetValue(hObject, pObject))
    return;

  // set object to inactive so components and children know that they shouldn't access the object anymore.
  pObject->m_Flags.Remove(ezObjectFlags::ActiveFlag | ezObjectFlags::ActiveState);

  // delete children
  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteObjectNow(it->GetHandle());
  }

  // delete attached components
  while (!pObject->m_Components.IsEmpty())
  {
    ezComponent* pComponent = pObject->m_Components[0];
    pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle());
  }
  EZ_ASSERT_DEV(pObject->m_Components.GetCount() == 0, "Components should already be removed");

  // fix parent and siblings
  UnlinkFromParent(pObject);

  // remove from global key tables
  SetObjectGlobalKey(pObject, ezHashedString());

  // invalidate (but preserve world index) and remove from id table
  pObject->m_InternalId.Invalidate();
  pObject->m_InternalId.m_WorldIndex = static_cast<ezUInt8>(m_uiIndex);

  m_Data.m_DeadObjects.Insert(pObject);
  EZ_VERIFY(m_Data.m_Objects.Remove(hObject), "Implementation error.");
}

void ezWorld::DeleteObjectDelayed(const ezGameObjectHandle& hObject)
{
  ezMsgDeleteGameObject msg;
  PostMessage(hObject, msg, ezTime::Zero());
}

ezComponentInitBatchHandle ezWorld::CreateComponentInitBatch(const char* szBatchName, bool bMustFinishWithinOneFrame /*= true*/)
{
  auto pInitBatch = EZ_NEW(GetAllocator(), ezInternal::WorldData::InitBatch, GetAllocator(), szBatchName, bMustFinishWithinOneFrame);
  return ezComponentInitBatchHandle(m_Data.m_InitBatches.Insert(pInitBatch));
}

void ezWorld::DeleteComponentInitBatch(const ezComponentInitBatchHandle& batch)
{
  auto& pInitBatch = m_Data.m_InitBatches[batch.GetInternalID()];
  EZ_ASSERT_DEV(pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty(),
    "Init batch has not been completely processed");
  m_Data.m_InitBatches.Remove(batch.GetInternalID());
}

void ezWorld::BeginAddingComponentsToInitBatch(const ezComponentInitBatchHandle& batch)
{
  EZ_ASSERT_DEV(m_Data.m_pCurrentInitBatch == m_Data.m_pDefaultInitBatch, "Nested init batches are not supported");
  m_Data.m_pCurrentInitBatch = m_Data.m_InitBatches[batch.GetInternalID()].Borrow();
}

void ezWorld::EndAddingComponentsToInitBatch(const ezComponentInitBatchHandle& batch)
{
  EZ_ASSERT_DEV(m_Data.m_InitBatches[batch.GetInternalID()] == m_Data.m_pCurrentInitBatch, "Init batch with id {} is currently not active",
    batch.GetInternalID().m_Data);
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

void ezWorld::SubmitComponentInitBatch(const ezComponentInitBatchHandle& batch)
{
  m_Data.m_InitBatches[batch.GetInternalID()]->m_bIsReady = true;
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

bool ezWorld::IsComponentInitBatchCompleted(const ezComponentInitBatchHandle& batch, double* pCompletionFactor /*= nullptr*/)
{
  auto& pInitBatch = m_Data.m_InitBatches[batch.GetInternalID()];
  EZ_ASSERT_DEV(pInitBatch->m_bIsReady, "Batch is not submitted yet");

  if (pCompletionFactor != nullptr)
  {
    if (pInitBatch->m_ComponentsToInitialize.IsEmpty())
    {
      double fStartSimCompletion = pInitBatch->m_ComponentsToStartSimulation.IsEmpty()
                                     ? 1.0
                                     : (double)pInitBatch->m_uiNextComponentToStartSimulation / pInitBatch->m_ComponentsToStartSimulation.GetCount();
      *pCompletionFactor = fStartSimCompletion * 0.5 + 0.5;
    }
    else
    {
      double fInitCompletion = pInitBatch->m_ComponentsToInitialize.IsEmpty()
                                 ? 1.0
                                 : (double)pInitBatch->m_uiNextComponentToInitialize / pInitBatch->m_ComponentsToInitialize.GetCount();
      *pCompletionFactor = fInitCompletion * 0.5;
    }
  }

  return pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty();
}

void ezWorld::CancelComponentInitBatch(const ezComponentInitBatchHandle& batch)
{
  auto& pInitBatch = m_Data.m_InitBatches[batch.GetInternalID()];
  pInitBatch->m_ComponentsToInitialize.Clear();
  pInitBatch->m_ComponentsToStartSimulation.Clear();
}
void ezWorld::PostMessage(
  const ezGameObjectHandle& receiverObject, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay, bool bRecursive) const
{
  // This method is allowed to be called from multiple threads.

  EZ_ASSERT_DEBUG((receiverObject.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in object id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = receiverObject.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent = false;
  metaData.m_uiRecursive = bRecursive;

  ezRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.GetSeconds() > 0.0)
  {
    ezMessage* pMsgCopy = pMsgRTTIAllocator->Clone<ezMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    ezMessage* pMsgCopy = pMsgRTTIAllocator->Clone<ezMessage>(&msg, m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void ezWorld::PostMessage(const ezComponentHandle& receiverComponent, const ezMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.

  EZ_ASSERT_DEBUG((receiverComponent.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in component id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = receiverComponent.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent = true;
  metaData.m_uiRecursive = false;

  ezRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.GetSeconds() > 0.0)
  {
    ezMessage* pMsgCopy = pMsgRTTIAllocator->Clone<ezMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    ezMessage* pMsgCopy = pMsgRTTIAllocator->Clone<ezMessage>(&msg, m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

const ezComponent* ezWorld::FindEventMsgHandler(ezEventMessage& msg, const ezGameObject* pSearchObject) const
{
  ezWorld* pWorld = const_cast<ezWorld*>(this);

  // walk the graph upwards until an object is found with an ezEventMessageHandlerComponent that handles this type of message
  {
    const ezGameObject* pCurrentObject = pSearchObject;

    while (pCurrentObject != nullptr)
    {
      const ezEventMessageHandlerComponent* pEventMessageHandlerComponent = nullptr;
      if (pCurrentObject->TryGetComponentOfBaseType(pEventMessageHandlerComponent))
      {
        if (pEventMessageHandlerComponent->HandlesEventMessage(msg))
        {
          return pEventMessageHandlerComponent;
        }

        // found an ezEventMessageHandlerComponent -> stop searching
        // even if it does not handle this type of message, we do not want to propagate the message to someone else
        return nullptr;
      }

      pCurrentObject = pCurrentObject->GetParent();
    }
  }

  // if no such object is found, check all objects that are registered as 'global event handlers'
  {
    auto globalEventMessageHandler = ezEventMessageHandlerComponent::GetAllGlobalEventHandler(this);
    for (auto hEventMessageHandlerComponent : globalEventMessageHandler)
    {
      ezEventMessageHandlerComponent* pEventMessageHandlerComponent = nullptr;
      if (pWorld->TryGetComponent(hEventMessageHandlerComponent, pEventMessageHandlerComponent))
      {
        if (pEventMessageHandlerComponent->HandlesEventMessage(msg))
        {
          return pEventMessageHandlerComponent;
        }
      }
    }
  }

  return nullptr;
}

void ezWorld::Update()
{
  CheckForWriteAccess();

  EZ_LOG_BLOCK(m_Data.m_sName.GetData());

  {
    ezStringBuilder sStatName;
    sStatName.Format("World Update/{0}/Game Object Count", m_Data.m_sName);

    ezStringBuilder sStatValue;
    ezStats::SetStat(sStatName, GetObjectCount());
  }

  m_Data.m_Clock.SetPaused(!m_Data.m_bSimulateWorld);
  m_Data.m_Clock.Update();

  // initialize phase
  {
    EZ_PROFILE_SCOPE("Initialize Phase");
    ProcessComponentsToInitialize();
    ProcessUpdateFunctionsToRegister();

    ProcessQueuedMessages(ezObjectMsgQueueType::AfterInitialized);
  }

  // pre-async phase
  {
    EZ_PROFILE_SCOPE("Pre-Async Phase");
    ProcessQueuedMessages(ezObjectMsgQueueType::NextFrame);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PreAsync]);
  }

  // async phase
  {
    // remove write marker but keep the read marker. Thus no one can mark the world for writing now. Only reading is allowed in async phase.
    m_Data.m_WriteThreadID = (ezThreadID)0;

    EZ_PROFILE_SCOPE("Async Phase");
    UpdateAsynchronous();

    // restore write marker
    m_Data.m_WriteThreadID = ezThreadUtils::GetCurrentThreadID();
  }

  // post-async phase
  {
    EZ_PROFILE_SCOPE("Post-Async Phase");
    ProcessQueuedMessages(ezObjectMsgQueueType::PostAsync);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PostAsync]);
  }

  // delete dead objects and update the object hierarchy
  {
    EZ_PROFILE_SCOPE("Delete Dead Objects");
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

    EZ_PROFILE_SCOPE("Update Transforms");
    m_Data.UpdateGlobalTransforms(fInvDelta);
  }

  // post-transform phase
  {
    EZ_PROFILE_SCOPE("Post-Transform Phase");
    ProcessQueuedMessages(ezObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezComponentManagerBase::UpdateFunctionDesc::Phase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  {
    EZ_PROFILE_SCOPE("Initialize Phase 2");
    // Only process the default init batch here since it contains the components created at runtime.
    // Also make sure that all initialization is finished after this call by giving it enough time.
    ProcessInitializationBatch(*m_Data.m_pDefaultInitBatch, ezTime::Now() + ezTime::Hours(10000));

    ProcessQueuedMessages(ezObjectMsgQueueType::AfterInitialized);
  }

  // Swap our double buffered stack allocator
  m_Data.m_StackAllocator.Swap();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

ezWorldModule* ezWorld::GetOrCreateModule(const ezRTTI* pRtti)
{
  CheckForWriteAccess();

  const ezWorldModuleTypeId uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId == 0xFFFF)
  {
    return nullptr;
  }

  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  ezWorldModule* pModule = m_Data.m_Modules[uiTypeId];
  if (pModule == nullptr)
  {
    pModule = ezWorldModuleFactory::GetInstance()->CreateWorldModule(uiTypeId, this);
    pModule->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;
    m_Data.m_ModulesToStartSimulation.PushBack(pModule);
  }

  return pModule;
}

void ezWorld::DeleteModule(const ezRTTI* pRtti)
{
  CheckForWriteAccess();

  const ezWorldModuleTypeId uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (ezWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      pModule->Deinitialize();
      DeregisterUpdateFunctions(pModule);
      EZ_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

ezWorldModule* ezWorld::GetModule(const ezRTTI* pRtti)
{
  CheckForWriteAccess();

  const ezWorldModuleTypeId uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

const ezWorldModule* ezWorld::GetModule(const ezRTTI* pRtti) const
{
  CheckForReadAccess();

  const ezWorldModuleTypeId uiTypeId = ezWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
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

  // TODO: the functions above send messages such as ezMsgChildrenChanged, which will not arrive for inactive components, is that a problem ?
  // 1) if a component was active before and now gets deactivated, it may not care about the message anymore anyway
  // 2) if a component was inactive before, it did not get the message, but upon activation it can update the state for which it needed the message
  // so probably it is fine, only components that were active and stay active need the message, and that will be the case
  pObject->UpdateActiveState(pNewParent == nullptr ? true : pNewParent->IsActive());
}

void ezWorld::LinkToParent(ezGameObject* pObject)
{
  EZ_ASSERT_DEBUG(
    pObject->m_NextSiblingIndex == 0 && pObject->m_PrevSiblingIndex == 0, "Object is either still linked to another parent or data was not cleared.");
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

    if (pParentObject->m_Flags.IsSet(ezObjectFlags::ChildChangesNotifications))
    {
      ezMsgChildrenChanged msg;
      msg.m_Type = ezMsgChildrenChanged::Type::ChildAdded;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
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

    if (pParentObject->m_Flags.IsSet(ezObjectFlags::ChildChangesNotifications))
    {
      ezMsgChildrenChanged msg;
      msg.m_Type = ezMsgChildrenChanged::Type::ChildRemoved;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
  }
}

void ezWorld::SetObjectGlobalKey(ezGameObject* pObject, const ezHashedString& sGlobalKey)
{
  if (m_Data.m_GlobalKeyToIdTable.Contains(sGlobalKey.GetHash()))
  {
    ezLog::Error("Can't set global key to '{0}' because an object with this global key already exists. Global keys have to be unique.", sGlobalKey);
    return;
  }

  const ezUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

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
  const ezUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

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
    ezComponentHandle hComponent(ezComponentId(entry.m_MetaData.m_uiReceiverObjectOrComponent));

    ezComponent* pReceiverComponent = nullptr;
    if (TryGetComponent(hComponent, pReceiverComponent))
    {
      pReceiverComponent->SendMessageInternal(*entry.m_pMessage, true);
    }
    else
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        ezLog::Warning(
          "ezWorld::ProcessQueuedMessage: Receiver ezComponent for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
  else
  {
    ezGameObjectHandle hObject(ezGameObjectId(entry.m_MetaData.m_uiReceiverObjectOrComponent));

    ezGameObject* pReceiverObject = nullptr;
    if (TryGetObject(hObject, pReceiverObject))
    {
      if (entry.m_MetaData.m_uiRecursive)
      {
        pReceiverObject->SendMessageRecursiveInternal(*entry.m_pMessage, true);
      }
      else
      {
        pReceiverObject->SendMessageInternal(*entry.m_pMessage, true);
      }
    }
    else
    {
#if EZ_ENABLED(EZ_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        ezLog::Warning(
          "ezWorld::ProcessQueuedMessage: Receiver ezGameObject for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
}

void ezWorld::ProcessQueuedMessages(ezObjectMsgQueueType::Enum queueType)
{
  EZ_PROFILE_SCOPE("Process Queued Messages");

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

      if (a.m_MetaData.m_uiReceiverData != b.m_MetaData.m_uiReceiverData)
        return a.m_MetaData.m_uiReceiverData < b.m_MetaData.m_uiReceiverData;

      if (a.m_uiMessageHash == 0)
      {
        a.m_uiMessageHash = a.m_pMessage->GetHash();
      }

      if (b.m_uiMessageHash == 0)
      {
        b.m_uiMessageHash = b.m_pMessage->GetHash();
      }

      return a.m_uiMessageHash < b.m_uiMessageHash;
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

  EZ_ASSERT_DEV(desc.m_Phase == ezComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_uiGranularity == 0,
    "Granularity must be 0 for synchronous update functions");
  EZ_ASSERT_DEV(desc.m_Phase != ezComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_DependsOn.GetCount() == 0,
    "Asynchronous update functions must not have dependencies");
  EZ_ASSERT_DEV(desc.m_Function.IsComparable(), "Delegates with captures are not allowed as ezWorld update functions.");

  m_Data.m_UpdateFunctionsToRegister.PushBack(desc);
}

void ezWorld::DeregisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];

  for (ezUInt32 i = updateFunctions.GetCount(); i-- > 0;)
  {
    if (updateFunctions[i].m_Function.IsEqualIfComparable(desc.m_Function))
    {
      updateFunctions.RemoveAtAndCopy(i);
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
        updateFunctions.RemoveAtAndCopy(i);
      }
    }
  }
}

void ezWorld::AddComponentToInitialize(ezComponentHandle hComponent)
{
  m_Data.m_pCurrentInitBatch->m_ComponentsToInitialize.PushBack(hComponent);
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
      EZ_PROFILE_SCOPE(updateFunction.m_sFunctionName);
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
      ezSharedPtr<ezInternal::WorldData::UpdateTask> pTask;
      if (uiCurrentTaskIndex < m_Data.m_UpdateTasks.GetCount())
      {
        pTask = m_Data.m_UpdateTasks[uiCurrentTaskIndex];
      }
      else
      {
        pTask = EZ_NEW(&m_Data.m_Allocator, ezInternal::WorldData::UpdateTask);
        m_Data.m_UpdateTasks.PushBack(pTask);
      }

      pTask->ConfigureTask(updateFunction.m_sFunctionName, ezTaskNesting::Maybe);
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

bool ezWorld::ProcessInitializationBatch(ezInternal::WorldData::InitBatch& batch, ezTime endTime)
{
  CheckForWriteAccess();

  if (!batch.m_ComponentsToInitialize.IsEmpty())
  {
    ezStringBuilder profileScopeName("Init ", batch.m_sName);
    EZ_PROFILE_SCOPE(profileScopeName);

    // Reserve for later use
    batch.m_ComponentsToStartSimulation.Reserve(batch.m_ComponentsToInitialize.GetCount());

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToInitialize < batch.m_ComponentsToInitialize.GetCount(); ++batch.m_uiNextComponentToInitialize)
    {
      ezComponentHandle hComponent = batch.m_ComponentsToInitialize[batch.m_uiNextComponentToInitialize];

      // if it is in the editor, the component might have been added and already deleted, without ever running the simulation
      ezComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
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

        batch.m_ComponentsToStartSimulation.PushBack(hComponent);
      }

      // Check if there is still time left to initialize more components
      if (ezTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToInitialize;
        return false;
      }
    }

    batch.m_ComponentsToInitialize.Clear();
    batch.m_uiNextComponentToInitialize = 0;
  }

  if (m_Data.m_bSimulateWorld)
  {
    ezStringBuilder startSimName("Start Sim ", batch.m_sName);
    EZ_PROFILE_SCOPE(startSimName);

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToStartSimulation < batch.m_ComponentsToStartSimulation.GetCount(); ++batch.m_uiNextComponentToStartSimulation)
    {
      ezComponentHandle hComponent = batch.m_ComponentsToStartSimulation[batch.m_uiNextComponentToStartSimulation];

      // if it is in the editor, the component might have been added and already deleted,  without ever running the simulation
      ezComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
        continue;

      if (pComponent->IsActiveAndInitialized())
      {
        pComponent->EnsureSimulationStarted();
      }

      // Check if there is still time left to initialize more components
      if (ezTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToStartSimulation;
        return false;
      }
    }

    batch.m_ComponentsToStartSimulation.Clear();
    batch.m_uiNextComponentToStartSimulation = 0;
  }

  return true;
}

void ezWorld::ProcessComponentsToInitialize()
{
  CheckForWriteAccess();

  if (m_Data.m_bSimulateWorld)
  {
    EZ_PROFILE_SCOPE("Modules Start Simulation");

    // Can't use foreach here because the array might be resized during iteration.
    for (ezUInt32 i = 0; i < m_Data.m_ModulesToStartSimulation.GetCount(); ++i)
    {
      m_Data.m_ModulesToStartSimulation[i]->OnSimulationStarted();
    }

    m_Data.m_ModulesToStartSimulation.Clear();
  }

  EZ_PROFILE_SCOPE("Initialize Components");

  ezTime endTime = ezTime::Now() + m_Data.m_MaxInitializationTimePerFrame;

  // First process all component init batches that have to finish within this frame
  for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
  {
    auto& pInitBatch = it.Value();
    if (pInitBatch->m_bIsReady && pInitBatch->m_bMustFinishWithinOneFrame)
    {
      ProcessInitializationBatch(*pInitBatch, ezTime::Now() + ezTime::Hours(10000));
    }
  }

  // If there is still time left process other component init batches
  if (ezTime::Now() < endTime)
  {
    for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
    {
      auto& pInitBatch = it.Value();
      if (!pInitBatch->m_bIsReady || pInitBatch->m_bMustFinishWithinOneFrame)
        continue;

      if (!ProcessInitializationBatch(*pInitBatch, endTime))
        return;
    }
  }
}

void ezWorld::ProcessUpdateFunctionsToRegister()
{
  CheckForWriteAccess();

  if (m_Data.m_UpdateFunctionsToRegister.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("Register update functions");

  while (!m_Data.m_UpdateFunctionsToRegister.IsEmpty())
  {
    const ezUInt32 uiNumFunctionsToRegister = m_Data.m_UpdateFunctionsToRegister.GetCount();

    for (ezUInt32 i = uiNumFunctionsToRegister; i-- > 0;)
    {
      if (RegisterUpdateFunctionInternal(m_Data.m_UpdateFunctionsToRegister[i]).Succeeded())
      {
        m_Data.m_UpdateFunctionsToRegister.RemoveAtAndCopy(i);
      }
    }

    EZ_ASSERT_DEV(m_Data.m_UpdateFunctionsToRegister.GetCount() < uiNumFunctionsToRegister,
      "No functions have been registered because the dependencies could not be found.");
  }
}

ezResult ezWorld::RegisterUpdateFunctionInternal(const ezWorldModule::UpdateFunctionDesc& desc)
{
  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];
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
    ezGameObject* pObject = m_Data.m_DeadObjects.GetIterator().Key();

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

      // The moved object might be deleted as well so we remove it from the dead objects set instead.
      // If that is not the case we remove the original object from the set.
      if (m_Data.m_DeadObjects.Remove(pMovedObject))
      {
        continue;
      }
    }

    m_Data.m_DeadObjects.Remove(pObject);
  }
}

void ezWorld::DeleteDeadComponents()
{
  while (!m_Data.m_DeadComponents.IsEmpty())
  {
    ezComponent* pComponent = m_Data.m_DeadComponents.GetIterator().Key();

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

      // The moved component might be deleted as well so we remove it from the dead components set instead.
      // If that is not the case we remove the original component from the set.
      if (m_Data.m_DeadComponents.Remove(pMovedComponent))
      {
        continue;
      }
    }

    m_Data.m_DeadComponents.Remove(pComponent);
  }
}

void ezWorld::PatchHierarchyData(ezGameObject* pObject, ezGameObject::TransformPreservation preserve)
{
  ezGameObject* pParent = pObject->GetParent();

  RecreateHierarchyData(pObject, pObject->IsDynamic());

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;

  if (preserve == ezGameObject::TransformPreservation::PreserveGlobal)
  {
    // SetGlobalTransform will internally trigger bounds update for static objects
    pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);
  }
  else
  {
    // Explicitly trigger transform AND bounds update, otherwise bounds would be outdated for static objects
    pObject->UpdateGlobalTransformAndBounds();
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

    pObject->m_uiHierarchyLevel = static_cast<ezUInt16>(uiNewHierarchyLevel);
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
