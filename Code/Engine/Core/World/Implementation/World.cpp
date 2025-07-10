#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/Stats.h>

ezStaticArray<ezWorld*, ezWorld::GetMaxNumWorlds()> ezWorld::s_Worlds;

static ezGameObjectHandle DefaultGameObjectReferenceResolver(const void* pData, ezComponentHandle hThis, ezStringView sProperty)
{
  EZ_IGNORE_UNUSED(hThis);
  EZ_IGNORE_UNUSED(sProperty);

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

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezWorld, ezNoBase, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_FUNCTIONS
  {
    EZ_SCRIPT_FUNCTION_PROPERTY(DeleteObjectDelayed, In, "GameObject", In, "DeleteEmptyParents")->AddAttributes(new ezFunctionArgumentAttributes(1, new ezDefaultValueAttribute(true))),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_TryGetObjectWithGlobalKey, In, "GlobalKey")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_SearchForObject, In, "SearchPath", In, "ReferenceObject")->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_GetClock)->AddFlags(ezPropertyFlags::PureFunction),
    EZ_SCRIPT_FUNCTION_PROPERTY(Reflection_GetRandomNumberGenerator)->AddFlags(ezPropertyFlags::PureFunction),
  }
  EZ_END_FUNCTIONS;
}
EZ_END_STATIC_REFLECTED_TYPE;
// clang-format on

ezWorld::ezWorld(ezWorldDesc& ref_desc)
  : m_Data(ref_desc)
{
  m_pUpdateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "WorldUpdate", ezTaskNesting::Never, ezMakeDelegate(&ezWorld::UpdateFromThread, this));
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  ezStringBuilder sb = ref_desc.m_sName.GetString();
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
    static_assert(GetMaxNumWorlds() == EZ_MAX_WORLDS);

    s_Worlds.PushBack(this);
  }

  SetGameObjectReferenceResolver(DefaultGameObjectReferenceResolver);
}

ezWorld::~ezWorld()
{
  SetWorldSimulationEnabled(false);

  EZ_LOCK(GetWriteMarker());
  m_Data.Clear();

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

  ezEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(this);
}

void ezWorld::SetCoordinateSystemProvider(const ezSharedPtr<ezCoordinateSystemProvider>& pProvider)
{
  EZ_ASSERT_DEV(pProvider != nullptr, "Coordinate System Provider must not be null");

  m_Data.m_pCoordinateSystemProvider = pProvider;
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;
}

// a super simple, but also efficient random number generator
inline static ezUInt32 NextStableRandomSeed(ezUInt32& ref_uiSeed)
{
  ref_uiSeed = 214013L * ref_uiSeed + 2531011L;
  return ((ref_uiSeed >> 16) & 0x7FFFF);
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
  newId.m_WorldIndex = ezGameObjectId::StorageType(m_uiIndex & (EZ_MAX_WORLDS - 1));

  // fill out some data
  pNewObject->m_InternalId = newId;
  pNewObject->m_Flags = ezObjectFlags::None;
  pNewObject->m_Flags.AddOrRemove(ezObjectFlags::Dynamic, bDynamic);
  pNewObject->m_Flags.AddOrRemove(ezObjectFlags::ActiveFlag, desc.m_bActiveFlag);
  pNewObject->m_sName = desc.m_sName;
  pNewObject->m_uiParentIndex = uiParentIndex;
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
  pTransformationData->m_globalTransform = ezSimdTransform::MakeIdentity();
#if EZ_ENABLED(EZ_GAMEOBJECT_VELOCITY)
  pTransformationData->m_lastGlobalTransform = ezSimdTransform::MakeIdentity();
  pTransformationData->m_uiLastGlobalTransformUpdateCounter = ezInvalidIndex;
#endif
  pTransformationData->m_localBounds = ezSimdBBoxSphere::MakeInvalid();
  pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(ezSimdFloat::MakeZero());
  pTransformationData->m_globalBounds = pTransformationData->m_localBounds;
  pTransformationData->m_hSpatialData.Invalidate();
  pTransformationData->m_uiSpatialDataCategoryBitmask = 0;
  pTransformationData->m_uiStableRandomSeed = desc.m_uiStableRandomSeed;

  // if seed is set to 0xFFFFFFFF, use the parent's seed to create a deterministic value for this object
  if (pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF && pTransformationData->m_pParentData != nullptr)
  {
    ezUInt32 seed = pTransformationData->m_pParentData->m_uiStableRandomSeed + pTransformationData->m_pParentData->m_pObject->GetChildCount();

    do
    {
      pTransformationData->m_uiStableRandomSeed = NextStableRandomSeed(seed);

    } while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF);
  }

  // if the seed is zero (or there was no parent to derive the seed from), assign a random value
  while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF)
  {
    pTransformationData->m_uiStableRandomSeed = GetRandomNumberGenerator().UInt();
  }

  pTransformationData->UpdateGlobalTransformNonRecursive(0);

  // link the transformation data to the game object
  pNewObject->m_pTransformationData = pTransformationData;

  // fix links
  LinkToParent(pNewObject);

  pNewObject->UpdateActiveState(pParentObject == nullptr ? true : pParentObject->IsActive());

  out_pObject = pNewObject;
  return ezGameObjectHandle(newId);
}

void ezWorld::DeleteObjectNow(const ezGameObjectHandle& hObject0, bool bAlsoDeleteEmptyParents /*= true*/)
{
  CheckForWriteAccess();

  ezGameObject* pObject = nullptr;
  if (!m_Data.m_Objects.TryGetValue(hObject0, pObject))
    return;

  ezGameObjectHandle hObject = hObject0;

  if (bAlsoDeleteEmptyParents)
  {
    ezGameObject* pParent = pObject->GetParent();

    while (pParent)
    {
      if (pParent->GetChildCount() != 1 || pParent->GetComponents().GetCount() != 0)
        break;

      pObject = pParent;

      pParent = pParent->GetParent();
    }

    hObject = pObject->GetHandle();
  }

  // inform external systems that we are about to delete this object
  m_Data.m_ObjectDeletionEvent.Broadcast(pObject);

  // set object to inactive so components and children know that they shouldn't access the object anymore.
  pObject->m_Flags.Remove(ezObjectFlags::ActiveFlag | ezObjectFlags::ActiveState);

  // delete children
  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteObjectNow(it->GetHandle(), false);
  }

  // delete attached components
  while (!pObject->m_Components.IsEmpty())
  {
    ezComponent* pComponent = pObject->m_Components[0];
    pComponent->DeleteComponent();
  }
  EZ_ASSERT_DEV(pObject->m_Components.GetCount() == 0, "Components should already be removed");

  // fix parent and siblings
  UnlinkFromParent(pObject);

  // remove from global key tables
  SetObjectGlobalKey(pObject, ezHashedString());

  // invalidate (but preserve world index) and remove from id table
  pObject->m_InternalId.Invalidate();
  pObject->m_InternalId.m_WorldIndex = m_uiIndex;

  m_Data.m_DeadObjects.Insert(pObject);
  EZ_VERIFY(m_Data.m_Objects.Remove(hObject), "Implementation error.");
}

void ezWorld::DeleteObjectDelayed(const ezGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents /*= true*/)
{
  ezMsgDeleteGameObject msg;
  msg.m_bDeleteEmptyParents = bAlsoDeleteEmptyParents;
  PostMessage(hObject, msg, ezTime::MakeZero());
}

ezComponentInitBatchHandle ezWorld::CreateComponentInitBatch(ezStringView sBatchName, bool bMustFinishWithinOneFrame /*= true*/)
{
  auto pInitBatch = EZ_NEW(GetAllocator(), ezInternal::WorldData::InitBatch, GetAllocator(), sBatchName, bMustFinishWithinOneFrame);
  return ezComponentInitBatchHandle(m_Data.m_InitBatches.Insert(pInitBatch));
}

void ezWorld::DeleteComponentInitBatch(const ezComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  EZ_IGNORE_UNUSED(pInitBatch);
  EZ_ASSERT_DEV(pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty(), "Init batch has not been completely processed");
  m_Data.m_InitBatches.Remove(hBatch.GetInternalID());
}

void ezWorld::BeginAddingComponentsToInitBatch(const ezComponentInitBatchHandle& hBatch)
{
  EZ_ASSERT_DEV(m_Data.m_pCurrentInitBatch == m_Data.m_pDefaultInitBatch, "Nested init batches are not supported");
  m_Data.m_pCurrentInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()].Borrow();
}

void ezWorld::EndAddingComponentsToInitBatch(const ezComponentInitBatchHandle& hBatch)
{
  EZ_ASSERT_DEV(m_Data.m_InitBatches[hBatch.GetInternalID()] == m_Data.m_pCurrentInitBatch, "Init batch with id {} is currently not active", hBatch.GetInternalID().m_Data);
  EZ_IGNORE_UNUSED(hBatch);
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

void ezWorld::SubmitComponentInitBatch(const ezComponentInitBatchHandle& hBatch)
{
  m_Data.m_InitBatches[hBatch.GetInternalID()]->m_bIsReady = true;
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

bool ezWorld::IsComponentInitBatchCompleted(const ezComponentInitBatchHandle& hBatch, double* pCompletionFactor /*= nullptr*/)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  EZ_ASSERT_DEV(pInitBatch->m_bIsReady, "Batch is not submitted yet");

  if (pCompletionFactor != nullptr)
  {
    if (pInitBatch->m_ComponentsToInitialize.IsEmpty())
    {
      if (m_Data.m_bSimulateWorld)
      {
        double fStartSimCompletion = pInitBatch->m_ComponentsToStartSimulation.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToStartSimulation / pInitBatch->m_ComponentsToStartSimulation.GetCount();
        *pCompletionFactor = fStartSimCompletion * 0.5 + 0.5;
      }
      else
      {
        *pCompletionFactor = 1.0;

        EZ_ASSERT_DEV(m_Data.m_pDefaultInitBatch != pInitBatch, "");

        m_Data.m_pDefaultInitBatch->m_ComponentsToStartSimulation.PushBackRange(pInitBatch->m_ComponentsToStartSimulation);
        pInitBatch->m_ComponentsToStartSimulation.Clear();
        return true;
      }
    }
    else
    {
      double fInitCompletion = pInitBatch->m_ComponentsToInitialize.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToInitialize / pInitBatch->m_ComponentsToInitialize.GetCount();

      if (m_Data.m_bSimulateWorld)
      {
        *pCompletionFactor = fInitCompletion * 0.5;
      }
      else
      {
        *pCompletionFactor = fInitCompletion;
      }
    }
  }

  return pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty();
}

void ezWorld::CancelComponentInitBatch(const ezComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  pInitBatch->m_ComponentsToInitialize.Clear();
  pInitBatch->m_ComponentsToStartSimulation.Clear();
}
void ezWorld::PostMessage(const ezGameObjectHandle& receiverObject, const ezMessage& msg, ezObjectMsgQueueType::Enum queueType, ezTime delay, bool bRecursive) const
{
  // This method is allowed to be called from multiple threads.

  EZ_ASSERT_DEBUG((receiverObject.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in object id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = receiverObject.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent = false;
  metaData.m_uiRecursive = bRecursive;

  if (m_Data.m_ProcessingMessageQueue == queueType)
  {
    delay = ezMath::Max(delay, ezTime::MakeFromMilliseconds(1));
  }

  ezRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
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

void ezWorld::PostMessage(const ezComponentHandle& hReceiverComponent, const ezMessage& msg, ezTime delay, ezObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.

  EZ_ASSERT_DEBUG((hReceiverComponent.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in component id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = hReceiverComponent.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent = true;
  metaData.m_uiRecursive = false;

  if (m_Data.m_ProcessingMessageQueue == queueType)
  {
    delay = ezMath::Max(delay, ezTime::MakeFromMilliseconds(1));
  }

  ezRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
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

void ezWorld::FindEventMsgHandlers(const ezMessage& msg, ezGameObject* pSearchObject, ezDynamicArray<ezComponent*>& out_components)
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void ezWorld::FindEventMsgHandlers(const ezMessage& msg, const ezGameObject* pSearchObject, ezDynamicArray<const ezComponent*>& out_components) const
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void ezWorld::Update()
{
  CheckForWriteAccess();

  EZ_LOG_BLOCK(m_Data.m_sName.GetData());

  {
    ezStringBuilder sStatName;
    sStatName.SetFormat("World Update/{0}/Game Object Count", m_Data.m_sName);

    ezStringBuilder sStatValue;
    ezStats::SetStat(sStatName, GetObjectCount());
  }

  ++m_Data.m_uiUpdateCounter;

  if (!m_Data.m_bSimulateWorld)
  {
    // only change the pause mode temporarily
    // so that user choices don't get overridden

    const bool bClockPaused = m_Data.m_Clock.GetPaused();
    m_Data.m_Clock.SetPaused(true);
    m_Data.m_Clock.Update();
    m_Data.m_Clock.SetPaused(bClockPaused);
  }
  else
  {
    m_Data.m_Clock.Update();
  }

  if (m_Data.m_pSpatialSystem != nullptr)
  {
    m_Data.m_pSpatialSystem->StartNewFrame();
  }

  // reload resources
  {
    EZ_PROFILE_SCOPE("Reload Resources");
    ProcessResourceReloadFunctions();
  }

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
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezWorldUpdatePhase::PreAsync]);
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
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezWorldUpdatePhase::PostAsync]);
  }

  // delete dead objects and update the object hierarchy
  {
    EZ_PROFILE_SCOPE("Delete Dead Objects");
    DeleteDeadObjects();
    DeleteDeadComponents();
  }

  // update transforms
  {
    EZ_PROFILE_SCOPE("Update Transforms");
    m_Data.UpdateGlobalTransforms();
  }

  // post-transform phase
  {
    EZ_PROFILE_SCOPE("Post-Transform Phase");
    ProcessQueuedMessages(ezObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[ezWorldUpdatePhase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  {
    EZ_PROFILE_SCOPE("Initialize Phase 2");
    // Only process the default init batch here since it contains the components created at runtime.
    // Also make sure that all initialization is finished after this call by giving it enough time.
    ProcessInitializationBatch(*m_Data.m_pDefaultInitBatch, ezTime::Now() + ezTime::MakeFromHours(10000));

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

    if (m_Data.m_bSimulateWorld)
    {
      pModule->OnSimulationStarted();
    }
    else
    {
      m_Data.m_ModulesToStartSimulation.PushBack(pModule);
    }
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

ezGameObject* ezWorld::Reflection_TryGetObjectWithGlobalKey(ezTempHashedString sGlobalKey)
{
  ezGameObject* pObject = nullptr;
  bool res = TryGetObjectWithGlobalKey(sGlobalKey, pObject);
  EZ_IGNORE_UNUSED(res);
  return pObject;
}

ezClock* ezWorld::Reflection_GetClock()
{
  return &m_Data.m_Clock;
}

ezRandom* ezWorld::Reflection_GetRandomNumberGenerator()
{
  return &m_Data.m_Random;
}

void ezWorld::SetParent(ezGameObject* pObject, ezGameObject* pNewParent, ezTransformPreservation::Enum preserve)
{
  EZ_ASSERT_DEV(pObject != pNewParent, "Object can't be its own parent!");
  EZ_ASSERT_DEV(pNewParent == nullptr || pObject->IsDynamic() || pNewParent->IsStatic(), "Can't attach a static object to a dynamic parent!");
  CheckForWriteAccess();

  if (GetObjectUnchecked(pObject->m_uiParentIndex) == pNewParent)
    return;

  UnlinkFromParent(pObject);
  // UnlinkFromParent does not clear these as they are still needed in DeleteObjectNow to allow deletes while iterating.
  pObject->m_uiNextSiblingIndex = 0;
  pObject->m_uiPrevSiblingIndex = 0;
  if (pNewParent != nullptr)
  {
    // Ensure that the parent's global transform is up-to-date otherwise the object's local transform will be wrong afterwards.
    pNewParent->UpdateGlobalTransform();

    pObject->m_uiParentIndex = pNewParent->m_InternalId.m_InstanceIndex;
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
  EZ_ASSERT_DEBUG(pObject->m_uiNextSiblingIndex == 0 && pObject->m_uiPrevSiblingIndex == 0, "Object is either still linked to another parent or data was not cleared.");
  if (ezGameObject* pParentObject = pObject->GetParent())
  {
    const ezUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (pParentObject->m_uiFirstChildIndex != 0)
    {
      pObject->m_uiPrevSiblingIndex = pParentObject->m_uiLastChildIndex;
      GetObjectUnchecked(pParentObject->m_uiLastChildIndex)->m_uiNextSiblingIndex = uiIndex;
    }
    else
    {
      pParentObject->m_uiFirstChildIndex = uiIndex;
    }

    pParentObject->m_uiLastChildIndex = uiIndex;
    pParentObject->m_uiChildCount++;

    pObject->m_pTransformationData->m_pParentData = pParentObject->m_pTransformationData;

    if (pObject->m_Flags.IsSet(ezObjectFlags::ParentChangesNotifications))
    {
      ezMsgParentChanged msg;
      msg.m_Type = ezMsgParentChanged::Type::ParentLinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

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

    if (uiIndex == pParentObject->m_uiFirstChildIndex)
      pParentObject->m_uiFirstChildIndex = pObject->m_uiNextSiblingIndex;

    if (uiIndex == pParentObject->m_uiLastChildIndex)
      pParentObject->m_uiLastChildIndex = pObject->m_uiPrevSiblingIndex;

    if (ezGameObject* pNextObject = GetObjectUnchecked(pObject->m_uiNextSiblingIndex))
      pNextObject->m_uiPrevSiblingIndex = pObject->m_uiPrevSiblingIndex;

    if (ezGameObject* pPrevObject = GetObjectUnchecked(pObject->m_uiPrevSiblingIndex))
      pPrevObject->m_uiNextSiblingIndex = pObject->m_uiNextSiblingIndex;

    pParentObject->m_uiChildCount--;
    pObject->m_uiParentIndex = 0;
    pObject->m_pTransformationData->m_pParentData = nullptr;

    if (pObject->m_Flags.IsSet(ezObjectFlags::ParentChangesNotifications))
    {
      ezMsgParentChanged msg;
      msg.m_Type = ezMsgParentChanged::Type::ParentUnlinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

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

ezStringView ezWorld::GetObjectGlobalKey(const ezGameObject* pObject) const
{
  const ezUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  const ezHashedString* pGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pGlobalKey))
  {
    return pGlobalKey->GetView();
  }

  return {};
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
        ezLog::Warning("ezWorld::ProcessQueuedMessage: Receiver ezComponent for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
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
        ezLog::Warning("ezWorld::ProcessQueuedMessage: Receiver ezGameObject for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
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

    m_Data.m_ProcessingMessageQueue = queueType;
    for (ezUInt32 i = 0; i < queue.GetCount(); ++i)
    {
      ProcessQueuedMessage(queue[i]);

      // no need to deallocate these messages, they are allocated through a frame allocator
    }
    m_Data.m_ProcessingMessageQueue = ezObjectMsgQueueType::COUNT;

    queue.Clear();
  }

  // timed messages
  {
    ezInternal::WorldData::MessageQueue& queue = m_Data.m_TimedMessageQueues[queueType];
    queue.Sort(MessageComparer());

    const ezTime now = m_Data.m_Clock.GetAccumulatedTime();

    m_Data.m_ProcessingMessageQueue = queueType;
    while (!queue.IsEmpty())
    {
      auto& entry = queue.Peek();
      if (entry.m_MetaData.m_Due > now)
        break;

      ProcessQueuedMessage(entry);

      EZ_DELETE(&m_Data.m_Allocator, entry.m_pMessage);

      queue.Dequeue();
    }
    m_Data.m_ProcessingMessageQueue = ezObjectMsgQueueType::COUNT;
  }
}

// static
template <typename World, typename GameObject, typename Component>
void ezWorld::FindEventMsgHandlers(World& world, const ezMessage& msg, GameObject pSearchObject, ezDynamicArray<Component>& out_components)
{
  using EventMessageHandlerComponentType = typename std::conditional<std::is_const<World>::value, const ezEventMessageHandlerComponent*, ezEventMessageHandlerComponent*>::type;

  out_components.Clear();

  // walk the graph upwards until an object is found with at least one ezComponent that handles this type of message
  {
    auto pCurrentObject = pSearchObject;

    while (pCurrentObject != nullptr)
    {
      bool bContinueSearch = true;
      for (auto pComponent : pCurrentObject->GetComponents())
      {
        if constexpr (std::is_const<World>::value == false)
        {
          pComponent->EnsureInitialized();
        }

        if (pComponent->HandlesMessage(msg))
        {
          out_components.PushBack(pComponent);
          bContinueSearch = false;
        }
        else
        {
          if constexpr (std::is_const<World>::value)
          {
            if (pComponent->IsInitialized() == false)
            {
              ezLog::Warning("Component of type '{}' was not initialized (yet) and thus might have reported an incorrect result in HandlesMessage(). "
                             "To allow this component to be automatically initialized at this point in time call the non-const variant of SendEventMessage.",
                pComponent->GetDynamicRTTI()->GetTypeName());
            }
          }

          // only continue to search on parent objects if all event handlers on the current object have the "pass through unhandled events" flag set.
          if (auto pEventMessageHandlerComponent = ezDynamicCast<EventMessageHandlerComponentType>(pComponent))
          {
            bContinueSearch &= pEventMessageHandlerComponent->GetPassThroughUnhandledEvents();
          }
        }
      }

      if (!bContinueSearch)
      {
        // stop searching as we found at least one ezEventMessageHandlerComponent or one doesn't have the "pass through" flag set.
        return;
      }

      pCurrentObject = pCurrentObject->GetParent();
    }
  }

  // if no components have been found, check all event handler components that are registered as 'global event handlers'
  if (out_components.IsEmpty())
  {
    auto globalEventMessageHandler = ezEventMessageHandlerComponent::GetAllGlobalEventHandler(&world);
    for (auto hEventMessageHandlerComponent : globalEventMessageHandler)
    {
      EventMessageHandlerComponentType pEventMessageHandlerComponent = nullptr;
      if (world.TryGetComponent(hEventMessageHandlerComponent, pEventMessageHandlerComponent))
      {
        if (pEventMessageHandlerComponent->HandlesMessage(msg))
        {
          out_components.PushBack(pEventMessageHandlerComponent);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void ezWorld::RegisterUpdateFunction(const ezComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  EZ_ASSERT_DEV(desc.m_Phase != ezWorldUpdatePhase::Async || desc.m_DependsOn.GetCount() == 0, "Asynchronous update functions must not have dependencies");
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

  for (ezUInt32 phase = ezWorldUpdatePhase::PreAsync; phase < ezWorldUpdatePhase::COUNT; ++phase)
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

  ezDynamicArrayBase<ezInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[ezWorldUpdatePhase::Async];

  ezUInt32 uiCurrentTaskIndex = 0;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    ezWorldModule* pModule = static_cast<ezWorldModule*>(updateFunction.m_Function.GetClassInstance());
    ezComponentManagerBase* pManager = ezDynamicCast<ezComponentManagerBase*>(pModule);

    // a world module can also register functions in the async phase so we want at least one task
    const ezUInt32 uiTotalCount = pManager != nullptr ? pManager->GetComponentCount() : 1;
    const ezUInt32 uiGranularity = (updateFunction.m_uiAsyncPhaseBatchSize != 0) ? updateFunction.m_uiAsyncPhaseBatchSize : uiTotalCount;

    ezUInt32 uiStartIndex = 0;
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

  // ensure that all components that are created during this batch (e.g. from prefabs)
  // will also get initialized within this batch
  m_Data.m_pCurrentInitBatch = &batch;
  EZ_SCOPE_EXIT(m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch);

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

      EZ_ASSERT_DEBUG(pComponent->GetOwner() != nullptr, "Component must have a valid owner");

      // make sure the object's transform is up to date before the component is initialized.
      pComponent->GetOwner()->UpdateGlobalTransform();

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
      ProcessInitializationBatch(*pInitBatch, ezTime::Now() + ezTime::MakeFromHours(10000));
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

    EZ_ASSERT_DEV(m_Data.m_UpdateFunctionsToRegister.GetCount() < uiNumFunctionsToRegister, "No functions have been registered because the dependencies could not be found.");
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

  updateFunctions.InsertAt(uiInsertionIndex, newFunction);

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

void ezWorld::PatchHierarchyData(ezGameObject* pObject, ezTransformPreservation::Enum preserve)
{
  ezGameObject* pParent = pObject->GetParent();

  RecreateHierarchyData(pObject, pObject->IsDynamic());

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;

  if (preserve == ezTransformPreservation::Enum::PreserveGlobal)
  {
    // SetGlobalTransform will internally trigger bounds update for static objects
    pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);
  }
  else
  {
    // Explicitly trigger transform AND bounds update, otherwise bounds would be outdated for static objects
    // Don't call pObject->UpdateGlobalTransformAndBounds() here since that would recursively update the parent global transform which is already up-to-date.
    pObject->m_pTransformationData->UpdateGlobalTransformNonRecursive(GetUpdateCounter());

    pObject->m_pTransformationData->UpdateGlobalBounds(GetSpatialSystem());
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

void ezWorld::ProcessResourceReloadFunctions()
{
  ResourceReloadContext context;
  context.m_pWorld = this;

  for (auto& hResource : m_Data.m_NeedReload)
  {
    if (m_Data.m_ReloadFunctions.TryGetValue(hResource, m_Data.m_TempReloadFunctions))
    {
      for (auto& data : m_Data.m_TempReloadFunctions)
      {
        EZ_VERIFY(data.m_hComponent.IsInvalidated() || TryGetComponent(data.m_hComponent, context.m_pComponent), "Reload function called on dead component");
        context.m_pUserData = data.m_pUserData;

        data.m_Func(context);
      }
    }
  }

  m_Data.m_NeedReload.Clear();
}

void ezWorld::SetMaxInitializationTimePerFrame(ezTime maxInitTime)
{
  CheckForWriteAccess();

  m_Data.m_MaxInitializationTimePerFrame = maxInitTime;
}

void ezWorld::SetGameObjectReferenceResolver(const ReferenceResolver& resolver)
{
  m_Data.m_GameObjectReferenceResolver = resolver;
}

const ezWorld::ReferenceResolver& ezWorld::GetGameObjectReferenceResolver() const
{
  return m_Data.m_GameObjectReferenceResolver;
}

void ezWorld::AddResourceReloadFunction(ezTypelessResourceHandle hResource, ezComponentHandle hComponent, void* pUserData, ResourceReloadFunc function)
{
  CheckForWriteAccess();

  if (hResource.IsValid() == false)
    return;

  auto& data = m_Data.m_ReloadFunctions[hResource].ExpandAndGetRef();
  data.m_hComponent = hComponent;
  data.m_pUserData = pUserData;
  data.m_Func = function;
}

void ezWorld::RemoveResourceReloadFunction(ezTypelessResourceHandle hResource, ezComponentHandle hComponent, void* pUserData)
{
  CheckForWriteAccess();

  ezInternal::WorldData::ReloadFunctionList* pReloadFunctions = nullptr;
  if (m_Data.m_ReloadFunctions.TryGetValue(hResource, pReloadFunctions))
  {
    for (ezUInt32 i = 0; i < pReloadFunctions->GetCount(); ++i)
    {
      auto& data = (*pReloadFunctions)[i];
      if (data.m_hComponent == hComponent && data.m_pUserData == pUserData)
      {
        pReloadFunctions->RemoveAtAndSwap(i);
        break;
      }
    }
  }
}

ezGameObject* ezWorld::SearchForObject(ezStringView sSearchPath, ezGameObject* pRefObj, const ezRTTI* pExpectedComponent)
{
  // Possible paths:
  //
  // rel/path
  // ../rel/path
  // ..
  // G:key/rel/path
  // P:parent/rel/path
  // G:key/P:parent/rel/path
  // G:key/../rel/path
  // G:key/..
  // P:parent/../rel/path

  // if the search string starts with "G:", the next part of the path is the global key of an object
  // in this case, this object is not the reference object anymore, instead the object with that global key is the reference object
  if (sSearchPath.TrimWordStart("G:"))
  {
    ezStringView sGlobalKey;

    if (const char* szSep = sSearchPath.FindSubString("/"))
    {
      sGlobalKey = ezStringView(sSearchPath.GetStartPointer(), szSep);
      sSearchPath.SetStartPosition(szSep + 1);
    }
    else
    {
      sGlobalKey = sSearchPath;
      sSearchPath = {};
    }

    if (!TryGetObjectWithGlobalKey(ezTempHashedString(sGlobalKey), pRefObj))
    {
      return nullptr;
    }
  }

  if (pRefObj == nullptr)
    return nullptr;

  // if the search string starts with "P:", the next part of the path is an object name of a parent object
  // of the reference object, so we search upwards until we find the object with that name
  if (sSearchPath.TrimWordStart("P:"))
  {
    ezStringView sParentName;

    if (const char* szSep = sSearchPath.FindSubString("/"))
    {
      sParentName = ezStringView(sSearchPath.GetStartPointer(), szSep);
      sSearchPath.SetStartPosition(szSep + 1);
    }
    else
    {
      sParentName = sSearchPath;
      sSearchPath = {};
    }

    const ezTempHashedString sStartName(sParentName);
    while (!pRefObj->HasName(sStartName))
    {
      pRefObj = pRefObj->GetParent();

      if (pRefObj == nullptr)
        return nullptr;
    }
  }

  // if the path contains "..", we go up one parent
  // this is only allowed at the start of the relative path section
  while (sSearchPath.TrimWordStart("../") || sSearchPath.TrimWordStart(".."))
  {
    pRefObj = pRefObj->GetParent();

    if (pRefObj == nullptr)
      return nullptr;
  }

  return pRefObj->SearchForChildByNameSequence(sSearchPath, pExpectedComponent);
}


const ezGameObject* ezWorld::SearchForObject(ezStringView sSearchPath, const ezGameObject* pReferenceObject /*= nullptr*/, const ezRTTI* pExpectedComponent /*= nullptr*/) const
{
  ezWorld* pThis = const_cast<ezWorld*>(this);
  ezGameObject* pRef = const_cast<ezGameObject*>(pReferenceObject);
  return pThis->SearchForObject(sSearchPath, pRef, pExpectedComponent);
}

EZ_STATICLINK_FILE(Core, Core_World_Implementation_World);
