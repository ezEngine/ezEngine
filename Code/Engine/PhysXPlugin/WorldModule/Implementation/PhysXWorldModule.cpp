#include <PhysXPlugin/PCH.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <Core/Messages/CollisionMessage.h>
#include <Core/World/World.h>

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
      ezPxTask* pTask = EZ_NEW(ezFrameAllocator::GetCurrentAllocator(), ezPxTask);
      pTask->m_pTask = &task;
      pTask->SetTaskName(task.getName());

      ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::EarlyThisFrame);
    }

    virtual PxU32 getWorkerCount() const override
    {
      return ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks);
    }
  };

  static ezPxCpuDispatcher s_CpuDispatcher;

  PxFilterFlags ezPxFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
  {
    // let triggers through
    if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
    {
      pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
      return PxFilterFlag::eDEFAULT;
    }

    pairFlags = (PxPairFlag::Enum)0;

    // trigger the contact callback for pairs (A,B) where
    // the filter mask of A contains the ID of B and vice versa.
    if ((filterData0.word0 & filterData1.word1) || (filterData1.word0 & filterData0.word1))
    {
      if (filterData0.word3 != 0 || filterData1.word3 != 0)
      {
        pairFlags |= (PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_CONTACT_POINTS);
      }

      if (PxFilterObjectIsKinematic(attributes0) && PxFilterObjectIsKinematic(attributes1))
      {
        pairFlags |= PxPairFlag::eDETECT_DISCRETE_CONTACT;
      }
      else
      {
        pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
      }

      return PxFilterFlag::eDEFAULT;
    }

    return PxFilterFlag::eKILL;
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
    if (pairHeader.flags & (PxContactPairHeaderFlag::eDELETED_ACTOR_0 | PxContactPairHeaderFlag::eDELETED_ACTOR_1))
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

  }
};

//////////////////////////////////////////////////////////////////////////

ezPhysXWorldModule::ezPhysXWorldModule(ezWorld* pWorld)
  : ezPhysicsWorldModuleInterface(pWorld)
  , m_pPxScene(nullptr)
  , m_pCharacterManager(nullptr)
  , m_pSimulationEventCallback(nullptr)
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

    desc.flags |= PxSceneFlag::eENABLE_ACTIVETRANSFORMS;
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

bool ezPhysXWorldModule::CastRay(const ezVec3& vStart, const ezVec3& vDir, float fMaxLen, ezUInt8 uiCollisionLayer, ezVec3& out_vHitPos, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitGameObject, ezSurfaceResourceHandle& out_hSurface)
{
  PxQueryFilterData filter;
  filter.data.setToDefault();
  filter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  filter.data.word0 = EZ_BIT(uiCollisionLayer);
  filter.data.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(uiCollisionLayer);
  filter.data.word2 = 0;
  filter.data.word3 = 0;

  EZ_PX_READ_LOCK(*m_pPxScene);

  ezPxQueryFilter QueryFilter;

  PxRaycastHit hit;
  if (m_pPxScene->raycastSingle(reinterpret_cast<const PxVec3&>(vStart), reinterpret_cast<const PxVec3&>(vDir),
                                  fMaxLen, PxHitFlag::ePOSITION | PxHitFlag::eNORMAL, hit,
                                  filter, &QueryFilter))
  {
    EZ_ASSERT_DEBUG(hit.shape != nullptr, "Raycast should have hit a shape");

    out_vHitPos = ezPxConversionUtils::ToVec3(hit.position);
    out_vHitNormal = ezPxConversionUtils::ToVec3(hit.normal);

    ezComponent* pComponent = ezPxUserData::GetComponent(hit.shape->userData);
    EZ_ASSERT_DEBUG(pComponent != nullptr, "Shape should have set a component as user data");

    out_hHitGameObject = pComponent->GetOwner()->GetHandle();

    EZ_ASSERT_DEBUG(!out_vHitPos.IsNaN(), "Raycast hit Position is NaN");
    EZ_ASSERT_DEBUG(!out_vHitNormal.IsNaN(), "Raycast hit Normal is NaN");

    auto pMaterial = hit.shape->getMaterialFromInternalFaceIndex(hit.faceIndex);

    if (pMaterial)
    {
      ezSurfaceResource* pSurface = ezPxUserData::GetSurfaceResource(pMaterial->userData);

      out_hSurface = ezSurfaceResourceHandle(pSurface);
    }

    return true;
  }

  return false;
}

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

bool ezPhysXWorldModule::SweepTestCapsule(const ezTransform& start, const ezVec3& vDir, float fCapsuleRadius, float fCapsuleHeight, float fDistance, ezUInt8 uiCollisionLayer, float& out_fDistance, ezVec3& out_Position, ezVec3& out_Normal)
{
  PxQueryFilterData filter;
  filter.data.setToDefault();
  filter.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;

  filter.data.word0 = EZ_BIT(uiCollisionLayer);
  filter.data.word1 = ezPhysX::GetSingleton()->GetCollisionFilterConfig().GetFilterMask(uiCollisionLayer);
  filter.data.word2 = 0;
  filter.data.word3 = 0;

  ezPxQueryFilter QueryFilter;

  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = {0}, Height = {1}", ezArgF(fCapsuleRadius, 2), ezArgF(fCapsuleHeight, 2));

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot.SetFromMat3(start.m_Rotation);
  qRot = qFixRot * qRot;

  PxTransform pxt;
  pxt.p = ezPxConversionUtils::ToVec3(start.m_vPosition);
  pxt.q = ezPxConversionUtils::ToQuat(qRot);


  EZ_PX_READ_LOCK(*m_pPxScene);

  ezPxSweepCallback closestHit;

  PxRaycastHit hit;
  if (!m_pPxScene->sweep(capsule, pxt, *reinterpret_cast<const PxVec3*>(&vDir), fDistance, closestHit, PxHitFlag::eDEFAULT, filter, &QueryFilter))
    return false;

  out_fDistance = closestHit.block.distance;
  out_Position = ezPxConversionUtils::ToVec3(closestHit.block.position);
  out_Normal = ezPxConversionUtils::ToVec3(closestHit.block.normal);
  //closestHit.block.shape;
  //closestHit.block.actor;


  return true;
}

void ezPhysXWorldModule::StartSimulation(const ezWorldModule::UpdateContext& context)
{
  if (ezPxSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<ezPxSettingsComponentManager>())
  {
    ezPxSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();
    if (pSettings != nullptr && pSettings->IsModified())
    {
      SetGravity(pSettings->GetObjectGravity(), pSettings->GetCharacterGravity());

      m_Settings = pSettings->GetSettings();
      m_AccumulatedTimeSinceUpdate.SetZero();

      pSettings->ResetModified();
    }
  }

  if (ezPxDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezPxDynamicActorComponentManager>())
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);

    pDynamicActorManager->UpdateKinematicActors();
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

    PxU32 numActiveTransforms = 0;
    const PxActiveTransform* pActiveTransforms = m_pPxScene->getActiveTransforms(numActiveTransforms);

    if (numActiveTransforms > 0)
    {
      pDynamicActorManager->UpdateDynamicActors(ezMakeArrayPtr(pActiveTransforms, numActiveTransforms));
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

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep || uiNumSubSteps == m_Settings.m_uiMaxSubSteps)
    {
      SimulateStep((float)tFixedStep.GetSeconds());

      m_AccumulatedTimeSinceUpdate -= tFixedStep;
      ++uiNumSubSteps;
    }
  }
  else if (m_Settings.m_SteppingMode == ezPxSteppingMode::SemiFixed)
  {
    float fDiff = (float)tDiff.GetSeconds();
    float fFixedStep = 1.0f / m_Settings.m_fFixedFrameRate;
    if (fFixedStep * m_Settings.m_uiMaxSubSteps < fDiff)
    {
      ///\todo add warning?
      fFixedStep = fDiff / m_Settings.m_uiMaxSubSteps;
    }

    while (fDiff > 0.0f)
    {
      float fDeltaTime = ezMath::Min(fFixedStep, fDiff);

      SimulateStep(fDeltaTime);

      fDiff -= fDeltaTime;
    }
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid stepping mode");
  }

  // not sure where exactly this needs to go
  m_pCharacterManager->computeInteractions((float)tDiff.GetSeconds());
}

void ezPhysXWorldModule::SimulateStep(float fDeltaTime)
{
  EZ_PX_WRITE_LOCK(*m_pPxScene);

  {
    EZ_PROFILE("Simulate");
    m_pPxScene->simulate(fDeltaTime);
  }

  {
    ///\todo execute tasks instead of waiting
    EZ_PROFILE("FetchResult");
    m_pPxScene->fetchResults(true);
  }
}


