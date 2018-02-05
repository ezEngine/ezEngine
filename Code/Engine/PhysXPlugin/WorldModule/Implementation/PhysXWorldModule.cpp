#include <PCH.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Core/Messages/TriggerMessage.h>
#include <Components/PxTriggerComponent.h>

EZ_IMPLEMENT_WORLD_MODULE(ezPhysXWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXWorldModule, 1, ezRTTINoAllocator);
// no properties or message handlers
EZ_END_DYNAMIC_REFLECTED_TYPE

namespace
{
  class ezPxTask : public ezTask
  {
  public:
    virtual void Execute() override
    {
      m_pTask->run();
      m_pTask->release();
      m_pTask = nullptr;
    }

    PxBaseTask* m_pTask;
  };

  class ezPxCpuDispatcher : public PxCpuDispatcher
  {
  public:
    ezPxCpuDispatcher()
    {
    }

    virtual void submitTask(PxBaseTask& task) override
    {
      ezPxTask* pTask = nullptr;
      //pTask = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezPxTask);

      {
        EZ_LOCK(m_Mutex);

        if (m_FreeTasks.IsEmpty())
        {
          pTask = &m_TaskStorage.ExpandAndGetRef();
          pTask->SetOnTaskFinished([this](ezTask* pTask)
          {
            FinishTask(static_cast<ezPxTask*>(pTask));
          });
        }
        else
        {
          pTask = m_FreeTasks.PeekBack();
          m_FreeTasks.PopBack();
        }
      }

      pTask->m_pTask = &task;
      pTask->SetTaskName(task.getName());

      ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::EarlyThisFrame);
    }

    virtual PxU32 getWorkerCount() const override
    {
      return ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks);
    }

    void FinishTask(ezPxTask* pTask)
    {
      EZ_LOCK(m_Mutex);
      m_FreeTasks.PushBack(pTask);
    }

    ezMutex m_Mutex;
    ezDeque<ezPxTask, ezStaticAllocatorWrapper> m_TaskStorage;
    ezDynamicArray<ezPxTask*, ezStaticAllocatorWrapper> m_FreeTasks;
  };

  static ezPxCpuDispatcher s_CpuDispatcher;

  PxFilterFlags ezPxFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
  {
    const bool kinematic0 = PxFilterObjectIsKinematic(attributes0);
    const bool kinematic1 = PxFilterObjectIsKinematic(attributes1);

    const bool static0 = PxGetFilterObjectType(attributes0) == PxFilterObjectType::eRIGID_STATIC;
    const bool static1 = PxGetFilterObjectType(attributes1) == PxFilterObjectType::eRIGID_STATIC;

    if ((kinematic0 || kinematic1) && (static0 || static1))
    {
      return PxFilterFlag::eSUPPRESS;
    }

    if (kinematic0 && kinematic1)
    {
      return PxFilterFlag::eSUPPRESS;
    }

    pairFlags = (PxPairFlag::Enum)0;

    // trigger the contact callback for pairs (A,B) where
    // the filter mask of A contains the ID of B and vice versa.
    if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
    {
      // let triggers through
      if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
      {
        pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
      }

      if (filterData0.word3 != 0 || filterData1.word3 != 0)
      {
        pairFlags |= (PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_CONTACT_POINTS);
      }

      pairFlags |= PxPairFlag::eCONTACT_DEFAULT;

      return PxFilterFlag::eDEFAULT;
    }

    return PxFilterFlag::eKILL;
  }

  class ezPxRaycastCallback : public PxRaycastCallback
  {
  public:
    ezPxRaycastCallback()
      : PxRaycastCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxRaycastHit* buffer, PxU32 nbHits) override
    {
      return false;
    }
  };

  class ezPxSweepCallback : public PxSweepCallback
  {
  public:
    ezPxSweepCallback()
      : PxSweepCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxSweepHit* buffer, PxU32 nbHits) override
    {
      return false;
    }
  };

  class ezPxOverlapCallback : public PxOverlapCallback
  {
  public:
    ezPxOverlapCallback()
      : PxOverlapCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxOverlapHit* buffer, PxU32 nbHits) override
    {
      return false;
    }
  };

  template <typename T>
  void FillHitResult(const T& hit, ezPhysicsHitResult& out_HitResult)
  {
    PxShape* pHitShape = hit.shape;
    EZ_ASSERT_DEBUG(pHitShape != nullptr, "Raycast should have hit a shape");

    out_HitResult.m_vPosition = ezPxConversionUtils::ToVec3(hit.position);
    out_HitResult.m_vNormal = ezPxConversionUtils::ToVec3(hit.normal);
    out_HitResult.m_fDistance = hit.distance;
    EZ_ASSERT_DEBUG(!out_HitResult.m_vPosition.IsNaN(), "Raycast hit Position is NaN");
    EZ_ASSERT_DEBUG(!out_HitResult.m_vNormal.IsNaN(), "Raycast hit Normal is NaN");

    {
      ezComponent* pShapeComponent = ezPxUserData::GetComponent(pHitShape->userData);
      EZ_ASSERT_DEBUG(pShapeComponent != nullptr, "Shape should have set a component as user data");
      out_HitResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
      out_HitResult.m_uiShapeId = pHitShape->getQueryFilterData().word2;
    }

    {
      ezComponent* pActorComponent = ezPxUserData::GetComponent(pHitShape->getActor()->userData);
      EZ_ASSERT_DEBUG(pActorComponent != nullptr, "Actor should have set a component as user data");
      out_HitResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }

    if (PxMaterial* pMaterial = pHitShape->getMaterialFromInternalFaceIndex(hit.faceIndex))
    {
      ezSurfaceResource* pSurface = ezPxUserData::GetSurfaceResource(pMaterial->userData);

      out_HitResult.m_hSurface = ezSurfaceResourceHandle(pSurface);
    }
  }
}

//////////////////////////////////////////////////////////////////////////

class ezPxSimulationEventCallback : public PxSimulationEventCallback
{
public:
  virtual void onConstraintBreak(PxConstraintInfo* constraints, PxU32 count) override
  {

  }

  virtual void onWake(PxActor** actors, PxU32 count) override
  {

  }

  virtual void onSleep(PxActor** actors, PxU32 count) override
  {

  }

  virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override
  {
    if (pairHeader.flags & (PxContactPairHeaderFlag::eREMOVED_ACTOR_0 | PxContactPairHeaderFlag::eREMOVED_ACTOR_1))
    {
      return;
    }

    ezCollisionMessage msg;
    msg.m_vPosition.SetZero();
    msg.m_vNormal.SetZero();
    msg.m_vImpulse.SetZero();

    float fNumContactPoints = 0.0f;

    ///\todo Not sure whether taking a simple average is the desired behavior here.
    for (ezUInt32 uiPairIndex = 0; uiPairIndex < nbPairs; ++uiPairIndex)
    {
      const PxContactPair& pair = pairs[uiPairIndex];

      PxContactPairPoint contactPointBuffer[16];
      ezUInt32 uiNumContactPoints = pair.extractContacts(contactPointBuffer, 16);

      for (ezUInt32 uiContactPointIndex = 0; uiContactPointIndex < uiNumContactPoints; ++uiContactPointIndex)
      {
        const PxContactPairPoint& point = contactPointBuffer[uiContactPointIndex];

        msg.m_vPosition += ezPxConversionUtils::ToVec3(point.position);
        msg.m_vNormal += ezPxConversionUtils::ToVec3(point.normal);
        msg.m_vImpulse += ezPxConversionUtils::ToVec3(point.impulse);

        fNumContactPoints += 1.0f;
      }
    }

    msg.m_vPosition /= fNumContactPoints;
    msg.m_vNormal.NormalizeIfNotZero();
    msg.m_vImpulse /= fNumContactPoints;

    const PxActor* pActorA = pairHeader.actors[0];
    const PxActor* pActorB = pairHeader.actors[1];

    const ezComponent* pComponentA = ezPxUserData::GetComponent(pActorA->userData);
    const ezComponent* pComponentB = ezPxUserData::GetComponent(pActorB->userData);

    const ezGameObject* pObjectA = nullptr;
    const ezGameObject* pObjectB = nullptr;

    if (pComponentA != nullptr)
    {
      pObjectA = pComponentA->GetOwner();

      msg.m_hObjectA = pObjectA->GetHandle();
      msg.m_hComponentA = pComponentA->GetHandle();
    }

    if (pComponentB != nullptr)
    {
      pObjectB = pComponentB->GetOwner();

      msg.m_hObjectB = pObjectB->GetHandle();
      msg.m_hComponentB = pComponentB->GetHandle();
    }


    if (pObjectA != nullptr)
    {
      pObjectA->PostMessage(msg, ezObjectMsgQueueType::PostTransform);
    }

    if (pObjectB != nullptr && pObjectB != pObjectA)
    {
      pObjectB->PostMessage(msg, ezObjectMsgQueueType::PostTransform);
    }
  }

  virtual void onTrigger(PxTriggerPair* pairs, PxU32 count) override
  {
    for (ezUInt32 i = 0; i < count; ++i)
    {
      const PxActor* pTriggerActor = pairs[i].triggerActor;
      const PxActor* pOtherActor = pairs[i].otherActor;

      const ezPxTriggerComponent* pTriggerComponent = ezPxUserData::GetTriggerComponent(pTriggerActor->userData);
      const ezComponent* pOtherComponent = ezPxUserData::GetComponent(pOtherActor->userData);

      if (pTriggerComponent != nullptr && pOtherComponent != nullptr)
      {
        ezTriggerState::Enum triggerState = pairs[i].status == PxPairFlag::eNOTIFY_TOUCH_FOUND ? ezTriggerState::Activated : ezTriggerState::Deactivated;
        pTriggerComponent->PostTriggerMessage(pOtherComponent, triggerState);
      }
    }
  }

  virtual void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count) override
  {

  }
};

//////////////////////////////////////////////////////////////////////////

ezPhysXWorldModule::ezPhysXWorldModule(ezWorld* pWorld)
  : ezPhysicsWorldModuleInterface(pWorld)
  , m_pPxScene(nullptr)
  , m_pCharacterManager(nullptr)
  , m_pSimulationEventCallback(nullptr)
  , m_uiNextShapeId(0)
  , m_FreeShapeIds(ezPhysX::GetSingleton()->GetAllocator())
  , m_ScratchMemory(ezPhysX::GetSingleton()->GetAllocator())
  , m_Settings()
  , m_SimulateTask("PhysX Simulate", ezMakeDelegate(&ezPhysXWorldModule::Simulate, this))
{

}

ezPhysXWorldModule::~ezPhysXWorldModule()
{

}

void ezPhysXWorldModule::Initialize()
{
  ezPhysX::GetSingleton()->LoadCollisionFilters();

  m_AccumulatedTimeSinceUpdate.SetZero();

  m_pSimulationEventCallback = EZ_DEFAULT_NEW(ezPxSimulationEventCallback);

  {
    PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
    desc.setToDefault(PxTolerancesScale());

    desc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    desc.flags |= PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS;
    desc.flags |= PxSceneFlag::eENABLE_KINEMATIC_PAIRS;
    desc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
    desc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;

    desc.gravity = ezPxConversionUtils::ToVec3(m_Settings.m_vObjectGravity);

    desc.cpuDispatcher = &s_CpuDispatcher;
    desc.filterShader = &ezPxFilterShader;
    desc.simulationEventCallback = m_pSimulationEventCallback;

    EZ_ASSERT_DEV(desc.isValid(), "PhysX scene description is invalid");
    m_pPxScene = ezPhysX::GetSingleton()->GetPhysXAPI()->createScene(desc);
    EZ_ASSERT_ALWAYS(m_pPxScene != nullptr, "Creating the PhysX scene failed");
  }

  m_pCharacterManager = PxCreateControllerManager(*m_pPxScene);

  {
    auto startSimDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPhysXWorldModule::StartSimulation, this);
    startSimDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    startSimDesc.m_bOnlyUpdateWhenSimulating = true;
    // Start physics simulation as late as possible in the first synchronous phase
    // so all kinematic objects have a chance to update their transform before.
    startSimDesc.m_fPriority = -100000.0f;

    RegisterUpdateFunction(startSimDesc);
  }

  {
    auto fetchResultsDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezPhysXWorldModule::FetchResults, this);
    fetchResultsDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    fetchResultsDesc.m_bOnlyUpdateWhenSimulating = true;
    // Fetch results as early as possible after async phase.
    fetchResultsDesc.m_fPriority = 100000.0f;

    RegisterUpdateFunction(fetchResultsDesc);
  }
}

void ezPhysXWorldModule::Deinitialize()
{
  m_pCharacterManager->release();
  m_pCharacterManager = nullptr;

  m_pPxScene->release();
  m_pPxScene = nullptr;

  EZ_DEFAULT_DELETE(m_pSimulationEventCallback);
}

void ezPhysXWorldModule::OnSimulationStarted()
{
  ezPhysX* pPhysX = ezPhysX::GetSingleton();

  pPhysX->LoadCollisionFilters();

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  pPhysX->StartupVDB();
#endif
}

ezUInt32 ezPhysXWorldModule::CreateShapeId()
{
  if (!m_FreeShapeIds.IsEmpty())
  {
    ezUInt32 uiShapeId = m_FreeShapeIds.PeekBack();
    m_FreeShapeIds.PopBack();

    return uiShapeId;
  }

  return m_uiNextShapeId++;
}

void ezPhysXWorldModule::DeleteShapeId(ezUInt32 uiShapeId)
{
  EZ_ASSERT_DEV(uiShapeId != ezInvalidIndex, "Trying to delete an invalid shape id");

  m_FreeShapeIds.PushBack(uiShapeId);
}

void ezPhysXWorldModule::SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity)
{
  m_Settings.m_vObjectGravity = objectGravity;
  m_Settings.m_vCharacterGravity = characterGravity;

  if (m_pPxScene)
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);

    m_pPxScene->setGravity(ezPxConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

bool ezPhysXWorldModule::CastRay(const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer,
                                 ezPhysicsHitResult& out_HitResult, bool bIncludeDynamic, ezUInt32 uiIgnoreShapeId) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(uiCollisionLayer, uiIgnoreShapeId);
  filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::ePREFILTER;

  if (bIncludeDynamic)
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  ezPxRaycastCallback closestHit;
  ezPxQueryFilter queryFilter;

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (GetPxScene()->raycast(ezPxConversionUtils::ToVec3(vStart), ezPxConversionUtils::ToVec3(vDir), fDistance,
                          closestHit, PxHitFlag::eDEFAULT, filterData, &queryFilter))
  {
    FillHitResult(closestHit.block, out_HitResult);

    return true;
  }

  return false;
}

bool ezPhysXWorldModule::SweepTestSphere(float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId /*= ezInvalidIndex*/) const
{
  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vStart, ezQuat::IdentityQuaternion());

  return SweepTest(sphere, transform, vDir, fDistance, uiCollisionLayer, out_HitResult, uiIgnoreShapeId);
}

bool ezPhysXWorldModule::SweepTestBox(ezVec3 vBoxExtends, const ezTransform& start, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId /*= ezInvalidIndex*/) const
{
  PxBoxGeometry box;
  box.halfExtents = ezPxConversionUtils::ToVec3(vBoxExtends * 0.5f);

  PxTransform transform = ezPxConversionUtils::ToTransform(start);

  return SweepTest(box, transform, vDir, fDistance, uiCollisionLayer, out_HitResult, uiIgnoreShapeId);
}

bool ezPhysXWorldModule::SweepTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& start, const ezVec3& vDir, float fDistance,
                                          ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId) const
{
  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = {0}, Height = {1}", ezArgF(fCapsuleRadius, 2), ezArgF(fCapsuleHeight, 2));

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot = start.m_qRotation;
  qRot = qFixRot * qRot;

  PxTransform transform = ezPxConversionUtils::ToTransform(start.m_vPosition, qRot);

  return SweepTest(capsule, transform, vDir, fDistance, uiCollisionLayer, out_HitResult, uiIgnoreShapeId);
}

bool ezPhysXWorldModule::SweepTest(const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezVec3& vDir, float fDistance,
                                   ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId) const
{
  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(uiCollisionLayer, uiIgnoreShapeId);
  filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  ezPxSweepCallback closestHit;
  ezPxQueryFilter queryFilter;

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (GetPxScene()->sweep(geometry, transform, ezPxConversionUtils::ToVec3(vDir), fDistance,
                        closestHit, PxHitFlag::eDEFAULT, filterData, &queryFilter))
  {
    FillHitResult(closestHit.block, out_HitResult);

    return true;
  }

  return false;
}

bool ezPhysXWorldModule::OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId /*= ezInvalidIndex*/) const
{
  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vPosition, ezQuat::IdentityQuaternion());

  return OverlapTest(sphere, transform, uiCollisionLayer, uiIgnoreShapeId);

}

bool ezPhysXWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& start, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId /*= ezInvalidIndex*/) const
{
  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = {0}, Height = {1}", ezArgF(fCapsuleRadius, 2), ezArgF(fCapsuleHeight, 2));

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot = start.m_qRotation;
  qRot = qFixRot * qRot;

  PxTransform transform = ezPxConversionUtils::ToTransform(start.m_vPosition, qRot);

  return OverlapTest(capsule, transform, uiCollisionLayer, uiIgnoreShapeId);
}

bool ezPhysXWorldModule::OverlapTest(const physx::PxGeometry& geometry, const physx::PxTransform& transform, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId) const
{
  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(uiCollisionLayer, uiIgnoreShapeId);
  filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER | PxQueryFlag::eANY_HIT;

  ezPxOverlapCallback closestHit;
  ezPxQueryFilter queryFilter;

  EZ_PX_READ_LOCK(*m_pPxScene);

  return GetPxScene()->overlap(geometry, transform, closestHit, filterData, &queryFilter);
}

static PxOverlapHit g_OverlapHits[256];

void ezPhysXWorldModule::QueryDynamicShapesInSphere(float fSphereRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezPhysicsOverlapResult& out_Results, ezUInt32 uiIgnoreShapeId /*= ezInvalidIndex*/) const
{
  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(uiCollisionLayer, uiIgnoreShapeId);

  // PxQueryFlag::eNO_BLOCK : All hits are reported as touching. Overrides eBLOCK returned from user filters with eTOUCH.
  // PxQueryFlag::eDYNAMIC : we ignore all static geometry
  filterData.flags = PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER | PxQueryFlag::eNO_BLOCK;

  ezPxQueryFilter queryFilter;

  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vPosition, ezQuat::IdentityQuaternion());

  PxOverlapBuffer allOverlaps(g_OverlapHits, EZ_ARRAY_SIZE(g_OverlapHits));

  EZ_PX_READ_LOCK(*m_pPxScene);

  GetPxScene()->overlap(sphere, transform, allOverlaps, filterData, &queryFilter);

  out_Results.m_Results.SetCountUninitialized(allOverlaps.nbTouches);
  for (ezUInt32 i = 0; i < allOverlaps.nbTouches; ++i)
  {
    {
      ezComponent* pShapeComponent = ezPxUserData::GetComponent(g_OverlapHits[i].shape->userData);
      out_Results.m_Results[i].m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
      out_Results.m_Results[i].m_uiShapeId = g_OverlapHits[i].shape->getQueryFilterData().word2;
    }

    {
      ezComponent* pActorComponent = ezPxUserData::GetComponent(g_OverlapHits[i].actor->userData);
      out_Results.m_Results[i].m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}

void ezPhysXWorldModule::StartSimulation(const ezWorldModule::UpdateContext& context)
{
  ezPxDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezPxDynamicActorComponentManager>();
  ezPxTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<ezPxTriggerComponentManager>();

  EZ_PX_WRITE_LOCK(*m_pPxScene);

  if (ezPxSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<ezPxSettingsComponentManager>())
  {
    ezPxSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();
    if (pSettings != nullptr && pSettings->IsModified())
    {
      SetGravity(pSettings->GetObjectGravity(), pSettings->GetCharacterGravity());

      if (pDynamicActorManager != nullptr && pSettings->IsModified(EZ_BIT(2))) // max depenetration velocity
      {
        pDynamicActorManager->UpdateMaxDepenetrationVelocity(pSettings->GetMaxDepenetrationVelocity());
      }

      m_Settings = pSettings->GetSettings();
      m_AccumulatedTimeSinceUpdate.SetZero();

      m_ScratchMemory.SetCountUninitialized(ezMemoryUtils::AlignSize(m_Settings.m_uiScratchMemorySize, 16u * 1024u));

      pSettings->ResetModified();
    }
  }

  if (pDynamicActorManager != nullptr)
  {
    pDynamicActorManager->UpdateKinematicActors();
  }

  if (pTriggerManager != nullptr)
  {
    pTriggerManager->UpdateKinematicActors();
  }

  m_SimulateTaskGroupId = ezTaskSystem::StartSingleTask(&m_SimulateTask, ezTaskPriority::EarlyThisFrame);
}

void ezPhysXWorldModule::FetchResults(const ezWorldModule::UpdateContext& context)
{
  {
    EZ_PROFILE("Wait for Simulate Task");
    ezTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  }

  if (ezPxDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezPxDynamicActorComponentManager>())
  {
    EZ_PX_READ_LOCK(*m_pPxScene);

    PxU32 numActiveActors = 0;
    PxActor** pActiveActors = m_pPxScene->getActiveActors(numActiveActors);

    if (numActiveActors > 0)
    {
      pDynamicActorManager->UpdateDynamicActors(ezMakeArrayPtr(pActiveActors, numActiveActors));
    }
  }
}

void ezPhysXWorldModule::Simulate()
{
  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  if (m_Settings.m_SteppingMode == ezPxSteppingMode::Variable)
  {
    SimulateStep((float)tDiff.GetSeconds());
  }
  else if (m_Settings.m_SteppingMode == ezPxSteppingMode::Fixed)
  {
    const ezTime tFixedStep = ezTime::Seconds(1.0 / m_Settings.m_fFixedFrameRate);

    m_AccumulatedTimeSinceUpdate += tDiff;
    ezUInt32 uiNumSubSteps = 0;

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep && uiNumSubSteps < m_Settings.m_uiMaxSubSteps)
    {
      SimulateStep((float)tFixedStep.GetSeconds());

      m_AccumulatedTimeSinceUpdate -= tFixedStep;
      ++uiNumSubSteps;
    }
  }
  else if (m_Settings.m_SteppingMode == ezPxSteppingMode::SemiFixed)
  {
    m_AccumulatedTimeSinceUpdate += tDiff;
    ezTime tFixedStep = ezTime::Seconds(1.0 / m_Settings.m_fFixedFrameRate);
    ezTime tMinStep = tFixedStep * 0.25;

    if (tFixedStep * m_Settings.m_uiMaxSubSteps < m_AccumulatedTimeSinceUpdate)
    {
      tFixedStep = m_AccumulatedTimeSinceUpdate / (double)m_Settings.m_uiMaxSubSteps;
    }

    while (m_AccumulatedTimeSinceUpdate > tMinStep)
    {
      ezTime tDeltaTime = ezMath::Min(tFixedStep, m_AccumulatedTimeSinceUpdate);

      SimulateStep((float)tDeltaTime.GetSeconds());

      m_AccumulatedTimeSinceUpdate -= tDeltaTime;
    }
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid stepping mode");
  }

  // not sure where exactly this needs to go
  //m_pCharacterManager->computeInteractions((float)tDiff.GetSeconds());
}

void ezPhysXWorldModule::SimulateStep(float fDeltaTime)
{
  EZ_PX_WRITE_LOCK(*m_pPxScene);

  {
    EZ_PROFILE("Simulate");
    m_pPxScene->simulate(fDeltaTime, nullptr, m_ScratchMemory.GetData(), m_ScratchMemory.GetCount());
  }

  {
    ///\todo execute tasks instead of waiting
    EZ_PROFILE("FetchResult");
    m_pPxScene->fetchResults(true);
  }
}




EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_WorldModule_Implementation_PhysXWorldModule);

