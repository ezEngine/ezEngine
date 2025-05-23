#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/Types/TagRegistry.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/Constraints/JoltFixedConstraintComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <Physics/Collision/CollisionCollectorImpl.h>
#include <Physics/Collision/Shape/Shape.h>
#include <RendererCore/Meshes/CustomMeshComponent.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezJoltWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezCVarBool cvar_JoltSimulationPause("Jolt.Simulation.Pause", false, ezCVarFlags::None, "Pauses the physics simulation.");

#ifdef JPH_DEBUG_RENDERER
ezCVarBool cvar_JoltDebugDrawConstraints("Jolt.DebugDraw.Constraints", false, ezCVarFlags::None, "Visualize physics constraints.");
ezCVarBool cvar_JoltDebugDrawConstraintLimits("Jolt.DebugDraw.ConstraintLimits", false, ezCVarFlags::None, "Visualize physics constraint limits.");
ezCVarBool cvar_JoltDebugDrawConstraintFrames("Jolt.DebugDraw.ConstraintFrames", false, ezCVarFlags::None, "Visualize physics constraint frames.");
ezCVarBool cvar_JoltDebugDrawBodies("Jolt.DebugDraw.Bodies", false, ezCVarFlags::None, "Visualize physics bodies.");
#endif

ezCVarBool cvar_JoltVisualizeGeometry("Jolt.Visualize.Geometry", false, ezCVarFlags::None, "Renders collision geometry.");
ezCVarBool cvar_JoltVisualizeGeometryExclusive("Jolt.Visualize.Exclusive", false, ezCVarFlags::Save, "Hides regularly rendered geometry.");
ezCVarFloat cvar_JoltVisualizeDistance("Jolt.Visualize.Distance", 30.0f, ezCVarFlags::Save, "How far away objects to visualize.");

ezJoltWorldModule::ezJoltWorldModule(ezWorld* pWorld)
  : ezPhysicsWorldModuleInterface(pWorld)
//, m_FreeObjectFilterIDs(ezJolt::GetSingleton()->GetAllocator()) // could use a proxy allocator to bin those
{
  m_pSimulateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "Jolt::Simulate", ezTaskNesting::Never, ezMakeDelegate(&ezJoltWorldModule::Simulate, this));
  m_pSimulateTask->ConfigureTask("Jolt Simulate", ezTaskNesting::Maybe);
}

ezJoltWorldModule::~ezJoltWorldModule() = default;

class ezJoltBodyActivationListener : public JPH::BodyActivationListener
{
public:
  virtual void OnBodyActivated(const JPH::BodyID& bodyID, JPH::uint64 inBodyUserData) override
  {
    const ezJoltUserData* pUserData = reinterpret_cast<const ezJoltUserData*>(inBodyUserData);
    if (ezJoltDynamicActorComponent* pActor = ezJoltUserData::GetDynamicActorComponent(pUserData))
    {
      m_pActiveActors->Insert(pActor);
    }

    if (ezJoltRagdollComponent* pActor = ezJoltUserData::GetRagdollComponent(pUserData))
    {
      (*m_pActiveRagdolls)[pActor]++;
    }

    if (ezJoltRopeComponent* pActor = ezJoltUserData::GetRopeComponent(pUserData))
    {
      (*m_pActiveRopes)[pActor]++;
    }
  }

  virtual void OnBodyDeactivated(const JPH::BodyID& bodyID, JPH::uint64 inBodyUserData) override
  {
    const ezJoltUserData* pUserData = reinterpret_cast<const ezJoltUserData*>(inBodyUserData);
    if (ezJoltActorComponent* pActor = ezJoltUserData::GetActorComponent(pUserData))
    {
      m_pActiveActors->Remove(pActor);
    }

    if (ezJoltRagdollComponent* pActor = ezJoltUserData::GetRagdollComponent(pUserData))
    {
      if (--(*m_pActiveRagdolls)[pActor] == 0)
      {
        m_pActiveRagdolls->Remove(pActor);
        m_pRagdollsPutToSleep->PushBack(pActor);
      }
    }

    if (ezJoltRopeComponent* pActor = ezJoltUserData::GetRopeComponent(pUserData))
    {
      if (--(*m_pActiveRopes)[pActor] == 0)
      {
        m_pActiveRopes->Remove(pActor);
      }
    }
  }

  ezSet<ezJoltDynamicActorComponent*>* m_pActiveActors = nullptr;
  ezMap<ezJoltRagdollComponent*, ezInt32>* m_pActiveRagdolls = nullptr; // value is a ref-count
  ezMap<ezJoltRopeComponent*, ezInt32>* m_pActiveRopes = nullptr;       // value is a ref-count
  ezDynamicArray<ezJoltRagdollComponent*>* m_pRagdollsPutToSleep = nullptr;
};

class ezJoltGroupFilter : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
  {
    const ezUInt64 id = static_cast<ezUInt64>(group1.GetGroupID()) << 32 | group2.GetGroupID();

    return !m_IgnoreCollisions.Contains(id);
  }

  ezHashSet<ezUInt64> m_IgnoreCollisions;
};

class ezJoltGroupFilterIgnoreSame : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& group1, const JPH::CollisionGroup& group2) const override
  {
    return group1.GetGroupID() != group2.GetGroupID();
  }
};

void ezJoltWorldModule::Deinitialize()
{
  m_pSystem = nullptr;
  m_pTempAllocator = nullptr;

  ezJoltBodyActivationListener* pActivationListener = reinterpret_cast<ezJoltBodyActivationListener*>(m_pActivationListener);
  EZ_DEFAULT_DELETE(pActivationListener);
  m_pActivationListener = nullptr;

  ezJoltContactListener* pContactListener = reinterpret_cast<ezJoltContactListener*>(m_pContactListener);
  EZ_DEFAULT_DELETE(pContactListener);
  m_pContactListener = nullptr;

  m_pGroupFilter->Release();
  m_pGroupFilter = nullptr;

  m_pGroupFilterIgnoreSame->Release();
  m_pGroupFilterIgnoreSame = nullptr;
}

class ezJoltTempAlloc : public JPH::TempAllocator
{
public:
  ezJoltTempAlloc(const char* szName)
    : m_ProxyAlloc(szName, ezFoundation::GetAlignedAllocator())
  {
    AddChunk(0);
    m_uiCurChunkIdx = 0;
  }

  ~ezJoltTempAlloc()
  {
    for (ezUInt32 i = 0; i < m_Chunks.GetCount(); ++i)
    {
      ClearChunk(i);
    }
  }

  virtual void* Allocate(JPH::uint inSize) override
  {
    if (inSize == 0)
      return nullptr;

    const ezUInt32 uiNeeded = ezMemoryUtils::AlignSize(inSize, 16u);

    while (true)
    {
      const ezUInt32 uiRemaining = m_Chunks[m_uiCurChunkIdx].m_uiSize - m_Chunks[m_uiCurChunkIdx].m_uiLastOffset;

      if (uiRemaining >= uiNeeded)
        break;

      AddChunk(uiNeeded);
    }

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];

    void* pRes = ezMemoryUtils::AddByteOffset(lastAlloc.m_pPtr, lastAlloc.m_uiLastOffset);
    lastAlloc.m_uiLastOffset += uiNeeded;
    return pRes;
  }

  virtual void Free(void* pInAddress, JPH::uint inSize) override
  {
    if (pInAddress == nullptr)
      return;

    const ezUInt32 uiAllocSize = ezMemoryUtils::AlignSize(inSize, 16u);

    auto& lastAlloc = m_Chunks[m_uiCurChunkIdx];
    lastAlloc.m_uiLastOffset -= uiAllocSize;

    if (lastAlloc.m_uiLastOffset == 0 && m_uiCurChunkIdx > 0)
    {
      // move back to the previous chunk
      --m_uiCurChunkIdx;
    }
  }

  struct Chunk
  {
    void* m_pPtr = nullptr;
    ezUInt32 m_uiSize = 0;
    ezUInt32 m_uiLastOffset = 0;
  };

  void AddChunk(ezUInt32 uiSize)
  {
    ++m_uiCurChunkIdx;

    if (m_uiCurChunkIdx < m_Chunks.GetCount())
      return;

    uiSize = ezMath::Max(uiSize, 1024u * 1024u);

    auto& alloc = m_Chunks.ExpandAndGetRef();
    alloc.m_pPtr = EZ_NEW_RAW_BUFFER(&m_ProxyAlloc, ezUInt8, uiSize);
    alloc.m_uiSize = uiSize;
  }

  void ClearChunk(ezUInt32 uiChunkIdx)
  {
    EZ_DELETE_RAW_BUFFER(&m_ProxyAlloc, m_Chunks[uiChunkIdx].m_pPtr);
    m_Chunks[uiChunkIdx].m_pPtr = nullptr;
    m_Chunks[uiChunkIdx].m_uiSize = 0;
    m_Chunks[uiChunkIdx].m_uiLastOffset = 0;
  }

  ezUInt32 m_uiCurChunkIdx = 0;
  ezHybridArray<Chunk, 16> m_Chunks;
  ezProxyAllocator m_ProxyAlloc;
};


void ezJoltWorldModule::Initialize()
{
  // TODO: it would be better if this were in OnSimulationStarted() to guarantee that the system is always initialized with the latest values
  // however, that doesn't work because ezJoltWorldModule is only created by calls to GetOrCreateWorldModule, where Initialize is called, but OnSimulationStarted
  // is queued and executed later

  // ensure the first element is reserved for 'invalid' objects
  m_AllocatedUserData.SetCount(1);

  UpdateSettingsCfg();

  ezStringBuilder tmp("Jolt-", GetWorld()->GetName());
  m_pTempAllocator = std::make_unique<ezJoltTempAlloc>(tmp);

  const uint32_t cMaxBodies = m_Settings.m_uiMaxBodies;
  const uint32_t cMaxContactConstraints = m_Settings.m_uiMaxBodies * 4;
  const uint32_t cMaxBodyPairs = cMaxContactConstraints * 10;
  const uint32_t cNumBodyMutexes = 0;

  m_pSystem = std::make_unique<JPH::PhysicsSystem>();
  m_pSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_ObjectToBroadphase, m_ObjectVsBroadphaseFilter, m_ObjectLayerPairFilter);

  {
    ezJoltBodyActivationListener* pListener = EZ_DEFAULT_NEW(ezJoltBodyActivationListener);
    m_pActivationListener = pListener;
    pListener->m_pActiveActors = &m_ActiveActors;
    pListener->m_pActiveRagdolls = &m_ActiveRagdolls;
    pListener->m_pActiveRopes = &m_ActiveRopes;
    pListener->m_pRagdollsPutToSleep = &m_RagdollsPutToSleep;
    m_pSystem->SetBodyActivationListener(pListener);
  }

  {
    ezJoltContactListener* pListener = EZ_DEFAULT_NEW(ezJoltContactListener);
    pListener->m_pWorld = GetWorld();
    m_pContactListener = pListener;
    m_pSystem->SetContactListener(pListener);
  }

  {
    m_pGroupFilter = new ezJoltGroupFilter();
    m_pGroupFilter->AddRef();
  }

  {
    m_pGroupFilterIgnoreSame = new ezJoltGroupFilterIgnoreSame();
    m_pGroupFilterIgnoreSame->AddRef();
  }
}

void ezJoltWorldModule::OnSimulationStarted()
{
  {
    auto startSimDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltWorldModule::StartSimulation, this);
    startSimDesc.m_Phase = ezWorldUpdatePhase::PreAsync;
    startSimDesc.m_bOnlyUpdateWhenSimulating = true;
    // Start physics simulation as late as possible in the first synchronous phase
    // so all kinematic objects have a chance to update their transform before.
    startSimDesc.m_fPriority = -100000.0f;

    RegisterUpdateFunction(startSimDesc);
  }

  {
    auto fetchResultsDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltWorldModule::FetchResults, this);
    fetchResultsDesc.m_Phase = ezWorldUpdatePhase::PostAsync;
    fetchResultsDesc.m_bOnlyUpdateWhenSimulating = true;
    // Fetch results as early as possible after async phase.
    fetchResultsDesc.m_fPriority = 100000.0f;

    RegisterUpdateFunction(fetchResultsDesc);
  }

  ezJoltCore::ReloadConfigs();

  UpdateSettingsCfg();
  ApplySettingsCfg();

  m_AccumulatedTimeSinceUpdate = ezTime::MakeZero();
}

ezUInt32 ezJoltWorldModule::CreateObjectFilterID()
{
  if (!m_FreeObjectFilterIDs.IsEmpty())
  {
    ezUInt32 uiObjectFilterID = m_FreeObjectFilterIDs.PeekBack();
    m_FreeObjectFilterIDs.PopBack();

    return uiObjectFilterID;
  }

  return m_uiNextObjectFilterID++;
}

void ezJoltWorldModule::DeleteObjectFilterID(ezUInt32& ref_uiObjectFilterID)
{
  if (ref_uiObjectFilterID == ezInvalidIndex)
    return;

  m_FreeObjectFilterIDs.PushBack(ref_uiObjectFilterID);

  ref_uiObjectFilterID = ezInvalidIndex;
}

ezUInt32 ezJoltWorldModule::AllocateUserData(ezJoltUserData*& out_pUserData)
{
  if (!m_FreeUserData.IsEmpty())
  {
    ezUInt32 uiIndex = m_FreeUserData.PeekBack();
    m_FreeUserData.PopBack();

    out_pUserData = &m_AllocatedUserData[uiIndex];
    return uiIndex;
  }

  out_pUserData = &m_AllocatedUserData.ExpandAndGetRef();
  return m_AllocatedUserData.GetCount() - 1;
}

void ezJoltWorldModule::DeallocateUserData(ezUInt32& ref_uiUserDataId)
{
  if (ref_uiUserDataId == ezInvalidIndex)
    return;

  m_AllocatedUserData[ref_uiUserDataId].Invalidate();

  m_FreeUserDataAfterSimulationStep.PushBack(ref_uiUserDataId);

  ref_uiUserDataId = ezInvalidIndex;
}

const ezJoltUserData& ezJoltWorldModule::GetUserData(ezUInt32 uiUserDataId) const
{
  EZ_ASSERT_DEBUG(uiUserDataId != ezInvalidIndex, "Invalid ezJoltUserData ID");

  return m_AllocatedUserData[uiUserDataId];
}

void ezJoltWorldModule::SetGravity(const ezVec3& vObjectGravity, const ezVec3& vCharacterGravity)
{
  m_Settings.m_vObjectGravity = vObjectGravity;
  m_Settings.m_vCharacterGravity = vCharacterGravity;

  if (m_pSystem)
  {
    m_pSystem->SetGravity(ezJoltConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

ezJoltForceId ezJoltWorldModule::AddOrUpdateForce(ezJoltForceId forceId, ezUInt32 uiBodyID, ezTime duration, const ezVec3& vForce)
{
  EZ_LOCK(m_ForcesMutex);

  ezJoltForce* pForce;
  if (m_Forces.TryGetValue(forceId, pForce))
  {
    pForce->m_tDisable = GetWorld()->GetClock().GetAccumulatedTime() + duration;
    pForce->m_vForce = vForce;
    return forceId;
  }
  else
  {
    ezJoltForce force;
    force.m_uiBodyID = uiBodyID;
    force.m_tDisable = GetWorld()->GetClock().GetAccumulatedTime() + duration;
    force.m_vForce = vForce;

    return m_Forces.Insert(force);
  }
}

void ezJoltWorldModule::ClearForce(ezJoltForceId id)
{
  EZ_LOCK(m_ForcesMutex);
  m_Forces.Remove(id);
}

void ezJoltWorldModule::UpdateForces()
{
  if (m_Forces.IsEmpty())
    return;

  EZ_LOCK(m_ForcesMutex);

  ezHybridArray<ezJoltForceId, 32> forcesToRemove;

  auto* pBodies = &m_pSystem->GetBodyInterface();
  const ezTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  for (auto it = m_Forces.GetIterator(); it.IsValid(); ++it)
  {
    ezJoltForce& force = it.Value();

    if (force.m_tDisable < tNow)
    {
      forcesToRemove.PushBack(it.Id());
      continue;
    }

    const JPH::BodyID bodyId(force.m_uiBodyID);

    if (!pBodies->IsAdded(bodyId))
      continue;

    pBodies->AddForce(bodyId, ezJoltConversionUtils::ToVec3(force.m_vForce));
  }

  for (ezJoltForceId id : forcesToRemove)
  {
    m_Forces.Remove(id);
  }
}

ezUInt32 ezJoltWorldModule::GetCollisionLayerByName(ezStringView sName) const
{
  return ezJoltCore::GetCollisionFilterConfig().GetFilterGroupByName(sName);
}

void ezJoltWorldModule::AddStaticCollisionBox(ezGameObject* pObject, ezVec3 vBoxSize)
{
  ezJoltStaticActorComponent* pActor = nullptr;
  ezJoltStaticActorComponent::CreateComponent(pObject, pActor);

  ezJoltShapeBoxComponent* pBox;
  ezJoltShapeBoxComponent::CreateComponent(pObject, pBox);
  pBox->SetHalfExtents(vBoxSize * 0.5f);
}

void ezJoltWorldModule::AddFixedJointComponent(ezGameObject* pOwner, const ezPhysicsWorldModuleInterface::FixedJointConfig& cfg)
{
  ezJoltFixedConstraintComponent* pConstraint = nullptr;
  m_pWorld->GetOrCreateComponentManager<ezJoltFixedConstraintComponentManager>()->CreateComponent(pOwner, pConstraint);
  pConstraint->SetActors(cfg.m_hActorA, cfg.m_LocalFrameA, cfg.m_hActorB, cfg.m_LocalFrameB);
}

ezBoundingBoxSphere ezJoltWorldModule::GetWorldSpaceBounds(ezGameObject* pOwner, ezUInt32 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const
{
  ezBoundingBoxSphere result = ezBoundingBoxSphere::MakeInvalid();

  ezJoltActorComponent* pActor = nullptr;
  if (pOwner->TryGetComponentOfBaseType(pActor))
  {
    ezUInt32 uiBodyID = pActor->GetJoltBodyID();
    auto& lockInterface = m_pSystem->GetBodyLockInterfaceNoLock();
    JPH::BodyLockRead bodyLock(lockInterface, JPH::BodyID(uiBodyID));
    if (bodyLock.Succeeded())
    {
      const auto& body = bodyLock.GetBody();

      if ((shapeTypes.GetValue() & EZ_BIT(body.GetBroadPhaseLayer().GetValue())) != 0 && ezJoltObjectLayerFilter(uiCollisionLayer).ShouldCollide(body.GetObjectLayer()))
      {
        const auto& aabb = body.GetWorldSpaceBounds();
        result = ezBoundingBoxSphere::MakeFromBox(ezBoundingBox::MakeFromMinMax(ezJoltConversionUtils::ToVec3(aabb.mMin), ezJoltConversionUtils::ToVec3(aabb.mMax)));
      }
    }
  }

  if (bIncludeChildObjects)
  {
    for (auto it = pOwner->GetChildren(); it.IsValid(); it.Next())
    {
      ezBoundingBoxSphere childBounds = GetWorldSpaceBounds(it, uiCollisionLayer, shapeTypes, bIncludeChildObjects);
      if (!childBounds.IsValid())
        continue;

      if (result.IsValid())
      {
        result.ExpandToInclude(childBounds);
      }
      else
      {
        result = childBounds;
      }
    }
  }

  return result;
}

void ezJoltWorldModule::QueueBodyToAdd(JPH::Body* pBody, bool bAwake)
{
  if (bAwake)
    m_BodiesToAddAndActivate.PushBack(pBody->GetID().GetIndexAndSequenceNumber());
  else
    m_BodiesToAdd.PushBack(pBody->GetID().GetIndexAndSequenceNumber());
}

void ezJoltWorldModule::EnableJoinedBodiesCollisions(ezUInt32 uiObjectFilterID1, ezUInt32 uiObjectFilterID2, bool bEnable)
{
  ezJoltGroupFilter* pFilter = static_cast<ezJoltGroupFilter*>(m_pGroupFilter);

  const ezUInt64 uiMask1 = static_cast<ezUInt64>(uiObjectFilterID1) << 32 | uiObjectFilterID2;
  const ezUInt64 uiMask2 = static_cast<ezUInt64>(uiObjectFilterID2) << 32 | uiObjectFilterID1;

  if (bEnable)
  {
    pFilter->m_IgnoreCollisions.Remove(uiMask1);
    pFilter->m_IgnoreCollisions.Remove(uiMask2);
  }
  else
  {
    pFilter->m_IgnoreCollisions.Insert(uiMask1);
    pFilter->m_IgnoreCollisions.Insert(uiMask2);
  }
}

void ezJoltWorldModule::ActivateCharacterController(ezJoltCharacterControllerComponent* pCharacter, bool bActivate)
{
  if (bActivate)
  {
    EZ_ASSERT_DEBUG(!m_ActiveCharacters.Contains(pCharacter), "ezJoltCharacterControllerComponent was activated more than once.");

    m_ActiveCharacters.PushBack(pCharacter);
  }
  else
  {
    if (!m_ActiveCharacters.RemoveAndSwap(pCharacter))
    {
      EZ_ASSERT_DEBUG(false, "ezJoltCharacterControllerComponent was deactivated more than once.");
    }
  }
}

void ezJoltWorldModule::CheckBreakableConstraints()
{
  ezWorld* pWorld = GetWorld();

  for (auto it = m_BreakableConstraints.GetIterator(); it.IsValid();)
  {
    ezJoltConstraintComponent* pConstraint;
    if (pWorld->TryGetComponent(*it, pConstraint) && pConstraint->IsActive())
    {
      if (pConstraint->ExceededBreakingPoint())
      {
        // notify interested parties, that this constraint is now broken
        ezMsgPhysicsJointBroke msg;
        msg.m_hJointObject = pConstraint->GetOwner()->GetHandle();
        pConstraint->GetOwner()->SendEventMessage(msg, pConstraint);

        // currently we don't track the broken state separately, we just remove the component
        pConstraint->DeleteComponent();
        it = m_BreakableConstraints.Remove(it);
      }
      else
      {
        ++it;
      }
    }
    else
    {
      it = m_BreakableConstraints.Remove(it);
    }
  }
}

void ezJoltWorldModule::FreeUserDataAfterSimulationStep()
{
  m_FreeUserData.PushBackRange(m_FreeUserDataAfterSimulationStep);
  m_FreeUserDataAfterSimulationStep.Clear();
}

void ezJoltWorldModule::StartSimulation(const ezWorldModule::UpdateContext& context)
{
  if (cvar_JoltSimulationPause)
    return;

  if (!m_BodiesToAdd.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAdd.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(ezUInt32));

    ezUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAdd.GetCount())
    {
      const ezUInt32 uiCount = m_BodiesToAdd.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAdd[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::DontActivate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAdd.Clear();
  }

  if (!m_BodiesToAddAndActivate.IsEmpty())
  {
    m_uiBodiesAddedSinceOptimize += m_BodiesToAddAndActivate.GetCount();

    static_assert(sizeof(JPH::BodyID) == sizeof(ezUInt32));

    ezUInt32 uiStartIdx = 0;

    while (uiStartIdx < m_BodiesToAddAndActivate.GetCount())
    {
      const ezUInt32 uiCount = m_BodiesToAddAndActivate.GetContiguousRange(uiStartIdx);

      JPH::BodyID* pIDs = reinterpret_cast<JPH::BodyID*>(&m_BodiesToAddAndActivate[uiStartIdx]);

      void* pHandle = m_pSystem->GetBodyInterface().AddBodiesPrepare(pIDs, uiCount);
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::Activate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAddAndActivate.Clear();
  }

  if (m_uiBodiesAddedSinceOptimize > 128)
  {
    // TODO: not clear whether this could be multi-threaded or done more efficiently somehow
    m_pSystem->OptimizeBroadPhase();
    m_uiBodiesAddedSinceOptimize = 0;
  }

  UpdateSettingsCfg();

  m_SimulatedTimeStep = CalculateUpdateSteps();

  if (m_UpdateSteps.IsEmpty())
    return;

  if (ezJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateKinematicActors(m_SimulatedTimeStep);
  }

  if (ezJoltQueryShapeActorComponentManager* pQueryShapesManager = GetWorld()->GetComponentManager<ezJoltQueryShapeActorComponentManager>())
  {
    pQueryShapesManager->UpdateMovingQueryShapes();
  }

  if (ezJoltTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<ezJoltTriggerComponentManager>())
  {
    pTriggerManager->UpdateMovingTriggers();
  }

  ApplyImpulses();
  UpdateForces();
  UpdateConstraints();

  m_SimulateTaskGroupId = ezTaskSystem::StartSingleTask(m_pSimulateTask, ezTaskPriority::EarlyThisFrame);
}

void ezJoltWorldModule::FetchResults(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("ezJoltWorldModule::FetchResults");

  {
    EZ_PROFILE_SCOPE("Wait for Simulate Task");
    ezTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  }

#ifdef JPH_DEBUG_RENDERER
  if (cvar_JoltDebugDrawConstraints)
    m_pSystem->DrawConstraints(ezJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintLimits)
    m_pSystem->DrawConstraintLimits(ezJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawConstraintFrames)
    m_pSystem->DrawConstraintReferenceFrame(ezJoltCore::s_pDebugRenderer.get());

  if (cvar_JoltDebugDrawBodies)
  {
    JPH::BodyManager::DrawSettings opt;
    opt.mDrawShape = true;
    opt.mDrawShapeWireframe = true;
    m_pSystem->DrawBodies(opt, ezJoltCore::s_pDebugRenderer.get());
  }

  ezJoltCore::DebugDraw(GetWorld());
#endif

  // Nothing to fetch if no simulation step was executed
  if (m_UpdateSteps.IsEmpty())
    return;

  ++m_uiJoltUpdateCounter;

  if (ezJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateDynamicActors();
  }

  for (auto pCharacter : m_ActiveCharacters)
  {
    pCharacter->Update(m_SimulatedTimeStep);
  }

  if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld()))
  {
    reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_ContactEvents.m_vMainCameraPosition = pView->GetCamera()->GetPosition();
  }

  reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_ContactEvents.SpawnPhysicsImpactReactions();
  reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsSlideReactions();
  reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_ContactEvents.UpdatePhysicsRollReactions();

  CheckBreakableConstraints();

  FreeUserDataAfterSimulationStep();

  DebugDrawGeometry();
}

ezTime ezJoltWorldModule::CalculateUpdateSteps()
{
  ezTime tSimulatedTimeStep = ezTime::MakeZero();
  m_AccumulatedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();
  m_UpdateSteps.Clear();

  if (m_Settings.m_SteppingMode == ezJoltSteppingMode::Variable)
  {
    // always do a single step with the entire time
    m_UpdateSteps.PushBack(m_AccumulatedTimeSinceUpdate);

    tSimulatedTimeStep = m_AccumulatedTimeSinceUpdate;
    m_AccumulatedTimeSinceUpdate = ezTime::MakeZero();
  }
  else if (m_Settings.m_SteppingMode == ezJoltSteppingMode::Fixed)
  {
    const ezTime tFixedStep = ezTime::MakeFromSeconds(1.0 / m_Settings.m_fFixedFrameRate);

    ezUInt32 uiNumSubSteps = 0;

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep && uiNumSubSteps < m_Settings.m_uiMaxSubSteps)
    {
      m_UpdateSteps.PushBack(tFixedStep);
      ++uiNumSubSteps;

      tSimulatedTimeStep += tFixedStep;
      m_AccumulatedTimeSinceUpdate -= tFixedStep;
    }
  }
  else if (m_Settings.m_SteppingMode == ezJoltSteppingMode::SemiFixed)
  {
    ezTime tFixedStep = ezTime::MakeFromSeconds(1.0 / m_Settings.m_fFixedFrameRate);
    const ezTime tMinStep = tFixedStep * 0.25;

    if (tFixedStep * m_Settings.m_uiMaxSubSteps < m_AccumulatedTimeSinceUpdate) // in case too much time has passed
    {
      // if taking N steps isn't sufficient to catch up to the passed time, increase the fixed time step accordingly
      tFixedStep = m_AccumulatedTimeSinceUpdate / (double)m_Settings.m_uiMaxSubSteps;
    }

    while (m_AccumulatedTimeSinceUpdate > tMinStep)
    {
      // prefer fixed time steps
      // but if at the end there is still more than tMinStep time left, do another step with the remaining time
      const ezTime tDeltaTime = ezMath::Min(tFixedStep, m_AccumulatedTimeSinceUpdate);

      m_UpdateSteps.PushBack(tDeltaTime);

      tSimulatedTimeStep += tDeltaTime;
      m_AccumulatedTimeSinceUpdate -= tDeltaTime;
    }
  }

  return tSimulatedTimeStep;
}

void ezJoltWorldModule::Simulate()
{
  if (m_UpdateSteps.IsEmpty())
    return;

  EZ_PROFILE_SCOPE("ezJoltWorldModule::Simulate");

  ezTime tDelta = m_UpdateSteps[0];
  ezUInt32 uiSteps = 1;

  m_RagdollsPutToSleep.Clear();

  for (ezUInt32 i = 1; i < m_UpdateSteps.GetCount(); ++i)
  {
    EZ_PROFILE_SCOPE("Physics Sim Step");

    if (m_UpdateSteps[i] == tDelta)
    {
      ++uiSteps;
    }
    else
    {
      // do a single Update call with multiple sub-steps, if possible
      // this saves a bit of time compared to just doing multiple Update calls

      m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, m_pTempAllocator.get(), ezJoltCore::GetJoltJobSystem());

      tDelta = m_UpdateSteps[i];
      uiSteps = 1;
    }
  }

  m_pSystem->Update((uiSteps * tDelta).AsFloatInSeconds(), uiSteps, m_pTempAllocator.get(), ezJoltCore::GetJoltJobSystem());
}

void ezJoltWorldModule::UpdateSettingsCfg()
{
  if (ezJoltSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<ezJoltSettingsComponentManager>())
  {
    ezJoltSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();

    if (pSettings != nullptr && pSettings->IsModified())
    {
      m_Settings = pSettings->GetSettings();
      pSettings->ResetModified();

      ApplySettingsCfg();
    }
  }
}

void ezJoltWorldModule::ApplySettingsCfg()
{
  SetGravity(m_Settings.m_vObjectGravity, m_Settings.m_vCharacterGravity);

  if (m_pSystem)
  {
    auto physicsSettings = m_pSystem->GetPhysicsSettings();
    physicsSettings.mPointVelocitySleepThreshold = m_Settings.m_fSleepVelocityThreshold;

    m_pSystem->SetPhysicsSettings(physicsSettings);
  }
}

void ezJoltWorldModule::UpdateConstraints()
{
  if (m_RequireUpdate.IsEmpty())
    return;

  ezJoltConstraintComponent* pComponent;
  for (auto& hComponent : m_RequireUpdate)
  {
    if (this->m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      pComponent->ApplySettings();
    }
  }

  m_RequireUpdate.Clear();
}

ezAtomicInteger32 s_iColMeshVisGeoCounter;

struct DebugVis
{
  const char* m_szMaterial = nullptr;
  ezColor m_Color;
};

const char* szMatSolid = "{ e6367876-ddb5-4149-ba80-180af553d463 }";       // Data/Base/Materials/Common/PhysicsColliders.ezMaterialAsset
const char* szMatTransparent = "{ ca43dda3-a28c-41fe-ae20-419182e56f87 }"; // Data/Base/Materials/Common/PhysicsCollidersTransparent.ezMaterialAsset
const char* szMatTwoSided = "{ b03df0e4-98b7-49ba-8413-d981014a77be }";    // Data/Base/Materials/Common/PhysicsCollidersSoft.ezMaterialAsset

static const DebugVis s_Vis[ezPhysicsShapeType::Count][2] =
  {
    // Static
    {
      {szMatSolid, ezColor::LightSkyBlue}, // non-kinematic
      {szMatSolid, ezColor::Red}           // kinematic
    },

    // Dynamic
    {
      {szMatSolid, ezColor::Gold},      // non-kinematic
      {szMatSolid, ezColor::DodgerBlue} // kinematic
    },

    // Query
    {
      {szMatTransparent, ezColor::GreenYellow.WithAlpha(0.5f)}, // non-kinematic
      {szMatTransparent, ezColor::GreenYellow.WithAlpha(0.5f)}  // kinematic
    },

    // Trigger
    {
      {szMatTransparent, ezColor::Purple.WithAlpha(0.3f)}, // non-kinematic
      {szMatTransparent, ezColor::Purple.WithAlpha(0.3f)}  // kinematic
    },

    // Character
    {
      {szMatTransparent, ezColor::DarkTurquoise.WithAlpha(0.5f)}, // non-kinematic
      {szMatTransparent, ezColor::DarkTurquoise.WithAlpha(0.5f)}  // kinematic
    },

    // Ragdoll
    {
      {szMatSolid, ezColor::DeepPink}, // non-kinematic
      {szMatSolid, ezColor::DeepPink}  // kinematic
    },

    // Rope
    {
      {szMatSolid, ezColor::MediumVioletRed}, // non-kinematic
      {szMatSolid, ezColor::MediumVioletRed}  // kinematic
    },

    // Cloth
    {
      {szMatTwoSided, ezColor::Crimson}, // non-kinematic
      {szMatTwoSided, ezColor::Red}      // kinematic
    },
};

void ezJoltWorldModule::DebugDrawGeometry()
{
  ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld());

  if (pView == nullptr)
    return;

  ++m_uiDebugGeoLastSeenCounter;

  const ezTag& tag = ezTagRegistry::GetGlobalRegistry().RegisterTag("PhysicsCollider");

  if (cvar_JoltVisualizeGeometry && cvar_JoltVisualizeGeometryExclusive)
  {
    // deactivate other geometry rendering
    pView->m_IncludeTags.Set(tag);
  }
  else
  {
    pView->m_IncludeTags.Remove(tag);
  }

  if (cvar_JoltVisualizeGeometry)
  {
    const ezVec3 vCenterPos = pView->GetCamera()->GetCenterPosition();

    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, ezPhysicsShapeType::Static, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, ezPhysicsShapeType::Dynamic, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, ezPhysicsShapeType::Query, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, ezPhysicsShapeType::Ragdoll, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, ezPhysicsShapeType::Trigger, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, ezPhysicsShapeType::Rope, tag);
    DebugDrawGeometry(vCenterPos, cvar_JoltVisualizeDistance, ezPhysicsShapeType::Cloth, tag);
  }

  for (auto it = m_DebugDrawComponents.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiLastSeenCounter < m_uiDebugGeoLastSeenCounter)
    {
      GetWorld()->DeleteObjectDelayed(it.Value().m_hObject);
      it = m_DebugDrawComponents.Remove(it);
    }
    else
    {
      ++it;
    }
  }

  for (auto it = m_DebugDrawShapeGeo.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_uiLastSeenCounter < m_uiDebugGeoLastSeenCounter)
    {
      it = m_DebugDrawShapeGeo.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void ezJoltWorldModule::DebugDrawGeometry(const ezVec3& vCenter, float fRadius, ezPhysicsShapeType::Enum shapeType, const ezTag& tag)
{
  const ezVec3 vAabbMin = vCenter - ezVec3(fRadius);
  const ezVec3 vAabbMax = vCenter + ezVec3(fRadius);

  JPH::AABox aabb;
  aabb.mMin = ezJoltConversionUtils::ToVec3(vAabbMin);
  aabb.mMax = ezJoltConversionUtils::ToVec3(vAabbMax);

  JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;

  ezJoltBroadPhaseLayerFilter broadphaseFilter(shapeType);
  JPH::ObjectLayerFilter objectFilterAll;
  JPH::BodyFilter bodyFilterAll;

  m_pSystem->GetNarrowPhaseQuery().CollectTransformedShapes(aabb, collector, broadphaseFilter, objectFilterAll, bodyFilterAll);

  auto* pBodies = &m_pSystem->GetBodyInterface();

  const int cMaxTriangles = 128;


  ezStaticArray<ezVec3, cMaxTriangles * 3> positionsTmp;
  positionsTmp.SetCountUninitialized(cMaxTriangles * 3);

  ezStaticArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp;
  materialsTmp.SetCountUninitialized(cMaxTriangles);

  ezHybridArray<ezVec3, cMaxTriangles * 3> positionsTmp2;
  ezHybridArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp2;

  for (const JPH::TransformedShape& ts : collector.mHits)
  {
    DebugBodyShapeKey key;
    key.m_uiBodyID = ts.mBodyID.GetIndexAndSequenceNumber();
    key.m_pShapePtr = ts.mShape.GetPtr();

    bool bExisted = false;
    auto& geo = m_DebugDrawComponents.FindOrAdd(key, &bExisted).Value();
    geo.m_uiLastSeenCounter = m_uiDebugGeoLastSeenCounter;

    DebugGeoShape& shapeGeo = m_DebugDrawShapeGeo[key.m_pShapePtr];
    shapeGeo.m_uiLastSeenCounter = m_uiDebugGeoLastSeenCounter;

    ezTransform objTrans;
    objTrans.m_vPosition = ezJoltConversionUtils::ToVec3(ts.mShapePositionCOM);
    objTrans.m_qRotation = ezJoltConversionUtils::ToQuat(ts.mShapeRotation);
    objTrans.m_vScale = ezJoltConversionUtils::ToVec3(ts.mShapeScale);

    if (bExisted)
    {
      ezGameObject* pObj;
      if (GetWorld()->TryGetObject(geo.m_hObject, pObj))
      {
        pObj->SetGlobalTransform(objTrans);
      }

      if (!geo.m_bMutableGeometry)
        continue;
    }

    JPH::BodyLockRead lock(m_pSystem->GetBodyLockInterface(), ts.mBodyID);
    if (!lock.Succeeded())
      continue;

    positionsTmp2.Clear();
    materialsTmp2.Clear();

    if (!shapeGeo.m_hMesh.IsValid() || (geo.m_bMutableGeometry && lock.GetBody().IsActive()))
    {
      shapeGeo.m_Bounds = ezBoundingBox::MakeInvalid();

      JPH::Shape::GetTrianglesContext ctx;
      ts.mShape->GetTrianglesStart(ctx, JPH::AABox::sBiggest(), JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::Vec3::sReplicate(1.0f));

      while (true)
      {
        const int triCount = ts.mShape->GetTrianglesNext(ctx, cMaxTriangles, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), materialsTmp.GetData());

        if (triCount == 0)
          break;

        positionsTmp2.PushBackRange(positionsTmp.GetArrayPtr().GetSubArray(0, triCount * 3));
        materialsTmp2.PushBackRange(materialsTmp.GetArrayPtr().GetSubArray(0, triCount));
      }

      if (positionsTmp2.GetCount() >= 3)
      {
        ezDynamicMeshBufferResourceDescriptor desc;
        desc.m_Topology = ezGALPrimitiveTopology::Triangles;
        desc.m_uiMaxVertices = positionsTmp2.GetCount();
        desc.m_uiMaxPrimitives = positionsTmp2.GetCount() / 3;
        desc.m_IndexType = ezGALIndexType::None;
        desc.m_bColorStream = false;

        ezStringBuilder sGuid;
        sGuid.SetFormat("ColMeshVisGeo_{}", s_iColMeshVisGeoCounter.Increment());

        if (!shapeGeo.m_hMesh.IsValid())
        {
          shapeGeo.m_hMesh = ezResourceManager::CreateResource<ezDynamicMeshBufferResource>(sGuid, std::move(desc));
        }

        ezResourceLock<ezDynamicMeshBufferResource> pMeshBuf(shapeGeo.m_hMesh, ezResourceAcquireMode::BlockTillLoaded);

        ezArrayPtr<ezDynamicMeshVertex> vertices = pMeshBuf->AccessVertexData();
        for (ezUInt32 vtxIdx = 0; vtxIdx < vertices.GetCount(); ++vtxIdx)
        {
          auto& vtx = vertices[vtxIdx];
          vtx.m_vPosition = positionsTmp2[vtxIdx];
          vtx.m_vTexCoord.SetZero();
          vtx.m_vEncodedNormal.SetZero();
          vtx.m_vEncodedTangent.SetZero();

          shapeGeo.m_Bounds.ExpandToInclude(vtx.m_vPosition);
        }
      }
    }

    if (bExisted)
      continue;

    ezGameObjectDesc gd;
    gd.m_bDynamic = true;
    gd.m_LocalPosition = objTrans.m_vPosition;
    gd.m_LocalRotation = objTrans.m_qRotation;
    gd.m_LocalScaling = objTrans.m_vScale;

    gd.m_Tags.Set(tag);
    ezGameObject* pObj;
    geo.m_hObject = GetWorld()->CreateObject(gd, pObj);
    geo.m_bMutableGeometry = lock.GetBody().IsSoftBody();

    ezCustomMeshComponent* pMesh;
    ezCustomMeshComponent::CreateComponent(pObj, pMesh);

    const bool bKinematic = (lock.GetBody().GetMotionType() == JPH::EMotionType::Kinematic);
    const auto& vis = s_Vis[ezMath::FirstBitLow((ezUInt32)shapeType)][bKinematic ? 1 : 0];

    pMesh->SetMeshResource(shapeGeo.m_hMesh);
    pMesh->SetBounds(shapeGeo.m_Bounds);
    pMesh->SetMaterialFile(vis.m_szMaterial);
    pMesh->SetColor(vis.m_Color);
  }
}

void ezJoltWorldModule::AddImpulse(ezUInt32 uiBodyID, const ezVec3& vImpulse, const ezVec3& vGlobalPosition)
{
  EZ_LOCK(m_ImpulsesMutex);
  auto& imp = m_Impulses.ExpandAndGetRef();
  imp.m_uiBodyID = uiBodyID;
  imp.m_vImpulse = vImpulse;
  imp.m_vGlobalPosition = vGlobalPosition;
}

void ezJoltWorldModule::ApplyImpulses()
{
  if (m_Impulses.IsEmpty())
    return;

  auto* pBodies = &m_pSystem->GetBodyInterface();
  ezHybridArray<ezJoltImpulse, 64> retain;

  EZ_LOCK(m_ImpulsesMutex);

  for (ezUInt32 i = 0; i < m_Impulses.GetCount(); ++i)
  {
    auto& imp = m_Impulses[i];

    const JPH::BodyID bodyId(imp.m_uiBodyID);

    if (bodyId.IsInvalid())
      continue;

    if (!pBodies->IsAdded(bodyId))
    {
      retain.PushBack(imp);
      continue;
    }

    pBodies->AddImpulse(bodyId, ezJoltConversionUtils::ToVec3(imp.m_vImpulse), ezJoltConversionUtils::ToVec3(imp.m_vGlobalPosition));
  }

  m_Impulses.Clear();

  for (auto& imp : retain)
  {
    m_Impulses.PushBack(imp);
  }
}

//////////////////////////////////////////////////////////////////

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezJoltNavmeshGeoWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezJoltNavmeshGeoWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezJoltNavmeshGeoWorldModule::ezJoltNavmeshGeoWorldModule(ezWorld* pWorld)
  : ezNavmeshGeoWorldModuleInterface(pWorld)
{
  m_pJoltModule = pWorld->GetOrCreateModule<ezJoltWorldModule>();
}

void ezJoltNavmeshGeoWorldModule::RetrieveGeometryInArea(ezUInt32 uiCollisionLayer, const ezBoundingBox& box, ezDynamicArray<ezNavmeshTriangle>& out_triangles) const
{
  const ezPhysicsQueryParameters params(uiCollisionLayer, ezPhysicsShapeType::Static);
  m_pJoltModule->QueryGeometryInBox(params, box, out_triangles);
}

EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltWorldModule);
