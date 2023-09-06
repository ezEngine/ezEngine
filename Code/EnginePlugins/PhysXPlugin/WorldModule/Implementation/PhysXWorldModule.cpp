#include <PhysXPlugin/PhysXPluginPCH.h>

#include <Core/Messages/CollisionMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/World/World.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <PhysXPlugin/Components/PxDynamicActorComponent.h>
#include <PhysXPlugin/Components/PxQueryShapeActorComponent.h>
#include <PhysXPlugin/Components/PxSettingsComponent.h>
#include <PhysXPlugin/Components/PxStaticActorComponent.h>
#include <PhysXPlugin/Components/PxTriggerComponent.h>
#include <PhysXPlugin/Joints/PxJointComponent.h>
#include <PhysXPlugin/Shapes/PxShapeBoxComponent.h>
#include <PhysXPlugin/Utilities/PxConversionUtils.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysX.h>
#include <PhysXPlugin/WorldModule/Implementation/PhysXSimulationEvents.h>
#include <PhysXPlugin/WorldModule/PhysXWorldModule.h>
#include <PxArticulation.h>
#include <RendererCore/AnimationSystem/AnimationPose.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>
#include <RendererCore/Components/CameraComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <pvd/PxPvdSceneClient.h>

// clang-format off
EZ_IMPLEMENT_WORLD_MODULE(ezPhysXWorldModule);
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezPhysXWorldModule, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

ezCVarBool cvar_PhysicsSimulationPause("Physics.Simulation.Pause", false, ezCVarFlags::None, "Pauses the physics simulation.");
ezCVarBool cvar_PhysicsDebugDrawEnable("Physics.DebugDraw.Enable", false, ezCVarFlags::None, "Enables physics debug visualizations.");
ezCVarFloat cvar_PhysicsDebugDrawSize("Physics.DebugDraw.Size", 0.2f, ezCVarFlags::Save, "Adjusts the size of physics debug visualizations (normals).");

namespace
{
  class ezPxTask final : public ezTask
  {
  public:
    virtual void Execute() override
    {
      m_pTask->run();
      m_pTask->release();
      m_pTask = nullptr;
    }

    PxBaseTask* m_pTask = nullptr;
  };

  class ezPxCpuDispatcher : public PxCpuDispatcher
  {
  public:
    ezPxCpuDispatcher() = default;

    virtual void submitTask(PxBaseTask& ref_task) override
    {
      ezSharedPtr<ezTask> pTask;

      {
        EZ_LOCK(m_Mutex);

        if (m_FreeTasks.IsEmpty())
        {
          m_TaskStorage.PushBack(EZ_DEFAULT_NEW(ezPxTask));
          pTask = m_TaskStorage.PeekBack();
        }
        else
        {
          pTask = m_FreeTasks.PeekBack();
          m_FreeTasks.PopBack();
        }
      }

      pTask->ConfigureTask(ref_task.getName(), ezTaskNesting::Never, [this](const ezSharedPtr<ezTask>& pTask)
        { FinishTask(pTask); });
      static_cast<ezPxTask*>(pTask.Borrow())->m_pTask = &ref_task;
      ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::EarlyThisFrame);
    }

    virtual PxU32 getWorkerCount() const override { return ezTaskSystem::GetWorkerThreadCount(ezWorkerThreadType::ShortTasks); }

    void FinishTask(const ezSharedPtr<ezTask>& pTask)
    {
      EZ_LOCK(m_Mutex);
      m_FreeTasks.PushBack(pTask);
    }

    ezMutex m_Mutex;
    ezDeque<ezSharedPtr<ezTask>, ezStaticAllocatorWrapper> m_TaskStorage;
    ezDynamicArray<ezSharedPtr<ezTask>, ezStaticAllocatorWrapper> m_FreeTasks;
  };

  static ezPxCpuDispatcher s_CpuDispatcher;

  PxFilterFlags ezPxFilterShader(PxFilterObjectAttributes attributes0, PxFilterData filterData0, PxFilterObjectAttributes attributes1, PxFilterData filterData1, PxPairFlags& ref_pairFlags, const void* pConstantBlock, PxU32 constantBlockSize)
  {
    const bool kinematic0 = PxFilterObjectIsKinematic(attributes0);
    const bool kinematic1 = PxFilterObjectIsKinematic(attributes1);

    const bool static0 = PxGetFilterObjectType(attributes0) == PxFilterObjectType::eRIGID_STATIC;
    const bool static1 = PxGetFilterObjectType(attributes1) == PxFilterObjectType::eRIGID_STATIC;

    if ((kinematic0 || kinematic1) && (static0 || static1))
    {
      return PxFilterFlag::eSUPPRESS;
    }

    // even though the documentation says that bodies in an articulation that are connected by a joint are generally ignored,
    // this is currently not true in reality
    // thus all shapes in an articulation (ragdoll) will collide with each other, which makes it impossible to build a proper ragdoll
    // the only way to prevent this, seems to be to disable ALL self-collision, unfortunately this also disables collisions with all other
    // articulations
    // TODO: this needs to be revisited with later PhysX versions
    // if (PxGetFilterObjectType(attributes0) == PxFilterObjectType::eARTICULATION && PxGetFilterObjectType(attributes1) == PxFilterObjectType::eARTICULATION)
    //{
    //  return PxFilterFlag::eSUPPRESS;
    //}

    ref_pairFlags = (PxPairFlag::Enum)0;

    // trigger the contact callback for pairs (A,B) where
    // the filter mask of A contains the ID of B and vice versa.
    if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
    {
      // let triggers through
      // note that triggers are typically kinematic
      // same for character controllers
      if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
      {
        ref_pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
        return PxFilterFlag::eDEFAULT;
      }

      // set when "report contacts" is enabled on a shape
      if (filterData0.word3 != 0 || filterData1.word3 != 0)
      {
        ref_pairFlags |= (PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_CONTACT_POINTS);
      }

      // if neither object is a trigger and both are kinematic, just suppress the contact
      if (kinematic0 && kinematic1)
      {
        return PxFilterFlag::eSUPPRESS;
      }

      ref_pairFlags |= PxPairFlag::eCONTACT_DEFAULT;
      ref_pairFlags |= PxPairFlag::eDETECT_CCD_CONTACT;

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

    virtual PxAgain processTouches(const PxRaycastHit* pBuffer, PxU32 hits) override { return false; }
  };

  class ezPxSweepCallback : public PxSweepCallback
  {
  public:
    ezPxSweepCallback()
      : PxSweepCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxSweepHit* pBuffer, PxU32 hits) override { return false; }
  };

  class ezPxOverlapCallback : public PxOverlapCallback
  {
  public:
    ezPxOverlapCallback()
      : PxOverlapCallback(nullptr, 0)
    {
    }

    virtual PxAgain processTouches(const PxOverlapHit* pBuffer, PxU32 hits) override { return false; }
  };

  template <typename T>
  void FillHitResult(const T& hit, ezPhysicsCastResult& out_result)
  {
    PxShape* pHitShape = hit.shape;
    EZ_ASSERT_DEBUG(pHitShape != nullptr, "Raycast should have hit a shape");

    out_result.m_vPosition = ezPxConversionUtils::ToVec3(hit.position);
    out_result.m_vNormal = ezPxConversionUtils::ToVec3(hit.normal);
    out_result.m_fDistance = hit.distance;
    EZ_ASSERT_DEBUG(!out_result.m_vPosition.IsNaN(), "Raycast hit Position is NaN");
    EZ_ASSERT_DEBUG(!out_result.m_vNormal.IsNaN(), "Raycast hit Normal is NaN");

    out_result.m_pInternalPhysicsShape = pHitShape;
    out_result.m_pInternalPhysicsActor = pHitShape->getActor();

    if (ezComponent* pShapeComponent = ezPxUserData::GetComponent(pHitShape->userData))
    {
      out_result.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
      out_result.m_uiObjectFilterID = pHitShape->getQueryFilterData().word2;
    }

    if (ezComponent* pActorComponent = ezPxUserData::GetComponent(pHitShape->getActor()->userData))
    {
      out_result.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }

    if (PxMaterial* pMaterial = pHitShape->getMaterialFromInternalFaceIndex(hit.faceIndex))
    {
      ezSurfaceResource* pSurface = ezPxUserData::GetSurfaceResource(pMaterial->userData);

      out_result.m_hSurface = ezSurfaceResourceHandle(pSurface);
    }
  }
} // namespace

EZ_DEFINE_AS_POD_TYPE(PxOverlapHit);
EZ_DEFINE_AS_POD_TYPE(PxRaycastHit);

//////////////////////////////////////////////////////////////////////////

ezPhysXWorldModule::ezPhysXWorldModule(ezWorld* pWorld)
  : ezPhysicsWorldModuleInterface(pWorld)
  , m_FreeShapeIds(ezPhysX::GetSingleton()->GetAllocator())
  , m_ScratchMemory(ezPhysX::GetSingleton()->GetAllocator())
  , m_Settings()
{
  m_pSimulateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "PhysX Simulate", ezTaskNesting::Maybe, ezMakeDelegate(&ezPhysXWorldModule::Simulate, this));

  m_ScratchMemory.SetCountUninitialized(ezMemoryUtils::AlignSize(m_Settings.m_uiScratchMemorySize, 16u * 1024u));
}

ezPhysXWorldModule::~ezPhysXWorldModule() = default;

void ezPhysXWorldModule::Initialize()
{
  ezPhysX::GetSingleton()->LoadCollisionFilters();

  m_AccumulatedTimeSinceUpdate = ezTime::MakeZero();

  m_pSimulationEventCallback = EZ_DEFAULT_NEW(ezPxSimulationEventCallback);
  m_pSimulationEventCallback->m_pWorld = GetWorld();

  {
    PxSceneDesc desc = PxSceneDesc(PxTolerancesScale());
    desc.setToDefault(PxTolerancesScale());

    desc.flags |= PxSceneFlag::eENABLE_ACTIVE_ACTORS;
    desc.flags |= PxSceneFlag::eEXCLUDE_KINEMATICS_FROM_ACTIVE_ACTORS;
    desc.kineKineFilteringMode = PxPairFilteringMode::eKEEP;
    desc.flags |= PxSceneFlag::eADAPTIVE_FORCE;
    desc.flags |= PxSceneFlag::eENABLE_CCD;

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    desc.flags |= PxSceneFlag::eREQUIRE_RW_LOCK;
#endif

    desc.gravity = ezPxConversionUtils::ToVec3(m_Settings.m_vObjectGravity);

    desc.cpuDispatcher = &s_CpuDispatcher;
    desc.filterShader = &ezPxFilterShader;
    desc.simulationEventCallback = m_pSimulationEventCallback;

    EZ_ASSERT_DEV(desc.isValid(), "PhysX scene description is invalid");
    m_pPxScene = ezPhysX::GetSingleton()->GetPhysXAPI()->createScene(desc);
    EZ_ASSERT_ALWAYS(m_pPxScene != nullptr, "Creating the PhysX scene failed");

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
    physx::PxPvdSceneClient* pvdClient = m_pPxScene->getScenePvdClient();
    if (pvdClient)
    {
      // pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
      // pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
      // pvdClient->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }
#endif
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

void ezPhysXWorldModule::DeleteShapeId(ezUInt32& ref_uiShapeId)
{
  if (ref_uiShapeId == ezInvalidIndex)
    return;

  m_FreeShapeIds.PushBack(ref_uiShapeId);

  ref_uiShapeId = ezInvalidIndex;
}

ezUInt32 ezPhysXWorldModule::AllocateUserData(ezPxUserData*& out_pUserData)
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

void ezPhysXWorldModule::DeallocateUserData(ezUInt32& ref_uiUserDataId)
{
  if (ref_uiUserDataId == ezInvalidIndex)
    return;

  m_AllocatedUserData[ref_uiUserDataId].Invalidate();

  m_FreeUserDataAfterSimulationStep.PushBack(ref_uiUserDataId);

  ref_uiUserDataId = ezInvalidIndex;
}

ezPxUserData& ezPhysXWorldModule::GetUserData(ezUInt32 uiUserDataId)
{
  EZ_ASSERT_DEBUG(uiUserDataId != ezInvalidIndex, "Invalid ezPxUserData ID");

  return m_AllocatedUserData[uiUserDataId];
}

void ezPhysXWorldModule::SetGravity(const ezVec3& vObjectGravity, const ezVec3& vCharacterGravity)
{
  m_Settings.m_vObjectGravity = vObjectGravity;
  m_Settings.m_vCharacterGravity = vCharacterGravity;

  if (m_pPxScene)
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);

    m_pPxScene->setGravity(ezPxConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

bool ezPhysXWorldModule::Raycast(ezPhysicsCastResult& out_result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection /*= ezPhysicsHitCollection::Closest*/) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreObjectFilterID);
  filterData.flags = PxQueryFlag::ePREFILTER;

  if (params.m_bIgnoreInitialOverlap)
  {
    // the postFilter will discard hits from initial overlaps (ie. when the raycast starts inside a shape)
    filterData.flags |= PxQueryFlag::ePOSTFILTER;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  if (collection == ezPhysicsHitCollection::Any)
  {
    filterData.flags |= PxQueryFlag::eANY_HIT;
  }

  ezPxRaycastCallback closestHit;
  ezPxQueryFilter queryFilter;
  queryFilter.m_bIncludeQueryShapes = params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Query);

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (m_pPxScene->raycast(ezPxConversionUtils::ToVec3(vStart), ezPxConversionUtils::ToVec3(vDir), fDistance, closestHit, PxHitFlag::eDEFAULT, filterData, &queryFilter))
  {
    FillHitResult(closestHit.block, out_result);

    return true;
  }

  return false;
}

bool ezPhysXWorldModule::RaycastAll(ezPhysicsCastResultArray& out_results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreObjectFilterID);

  // PxQueryFlag::eNO_BLOCK : All hits are reported as touching. Overrides eBLOCK returned from user filters with eTOUCH.
  filterData.flags = PxQueryFlag::ePREFILTER | PxQueryFlag::eNO_BLOCK;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  ezArrayPtr<PxRaycastHit> raycastHits = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), PxRaycastHit, 256);
  PxRaycastBuffer allHits(raycastHits.GetPtr(), raycastHits.GetCount());

  ezPxQueryFilter queryFilter;
  queryFilter.m_bIncludeQueryShapes = params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Query);

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (m_pPxScene->raycast(ezPxConversionUtils::ToVec3(vStart), ezPxConversionUtils::ToVec3(vDir), fDistance, allHits, PxHitFlag::eDEFAULT | PxHitFlag::eMESH_MULTIPLE | PxHitFlag::eMESH_BOTH_SIDES, filterData, &queryFilter))
  {
    out_results.m_Results.SetCount(allHits.nbTouches);

    for (ezUInt32 i = 0; i < allHits.nbTouches; ++i)
    {
      FillHitResult(allHits.touches[i], out_results.m_Results[i]);
    }

    return true;
  }

  return false;
}

bool ezPhysXWorldModule::SweepTestSphere(ezPhysicsCastResult& out_result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vStart, ezQuat::MakeIdentity());

  return SweepTest(out_result, sphere, transform, vDir, fDistance, params, collection);
}

bool ezPhysXWorldModule::SweepTestBox(ezPhysicsCastResult& out_result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxBoxGeometry box;
  box.halfExtents = ezPxConversionUtils::ToVec3(vBoxExtends * 0.5f);

  return SweepTest(out_result, box, ezPxConversionUtils::ToTransform(transform), vDir, fDistance, params, collection);
}

bool ezPhysXWorldModule::SweepTestCapsule(ezPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = {0}, Height = {1}", ezArgF(fCapsuleRadius, 2), ezArgF(fCapsuleHeight, 2));

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qFixRot * qRot;

  return SweepTest(out_result, capsule, ezPxConversionUtils::ToTransform(transform.m_vPosition, qRot), vDir, fDistance, params, collection);
}

bool ezPhysXWorldModule::SweepTest(ezPhysicsCastResult& out_Result, const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreObjectFilterID);

  filterData.flags = PxQueryFlag::ePREFILTER;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  if (collection == ezPhysicsHitCollection::Any)
  {
    filterData.flags |= PxQueryFlag::eANY_HIT;
  }

  ezPxSweepCallback closestHit;
  ezPxQueryFilter queryFilter;
  queryFilter.m_bIncludeQueryShapes = params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Query);

  EZ_PX_READ_LOCK(*m_pPxScene);

  if (m_pPxScene->sweep(geometry, transform, ezPxConversionUtils::ToVec3(vDir), fDistance, closestHit, PxHitFlag::eDEFAULT, filterData, &queryFilter))
  {
    FillHitResult(closestHit.block, out_Result);

    return true;
  }

  return false;
}

bool ezPhysXWorldModule::OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const
{
  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vPosition, ezQuat::MakeIdentity());

  return OverlapTest(sphere, transform, params);
}

bool ezPhysXWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  PxCapsuleGeometry capsule;
  capsule.radius = fCapsuleRadius;
  capsule.halfHeight = fCapsuleHeight * 0.5f;
  EZ_ASSERT_DEBUG(capsule.isValid(), "Invalid capsule parameter. Radius = {0}, Height = {1}", ezArgF(fCapsuleRadius, 2), ezArgF(fCapsuleHeight, 2));

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(0, 1, 0), ezAngle::MakeFromDegree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qFixRot * qRot;

  return OverlapTest(capsule, ezPxConversionUtils::ToTransform(transform.m_vPosition, qRot), params);
}

bool ezPhysXWorldModule::OverlapTest(const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezPhysicsQueryParameters& params) const
{
  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreObjectFilterID);

  filterData.flags = PxQueryFlag::ePREFILTER | PxQueryFlag::eANY_HIT;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  ezPxOverlapCallback closestHit;
  ezPxQueryFilter queryFilter;
  queryFilter.m_bIncludeQueryShapes = params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Query);

  EZ_PX_READ_LOCK(*m_pPxScene);

  return m_pPxScene->overlap(geometry, transform, closestHit, filterData, &queryFilter);
}

void ezPhysXWorldModule::QueryShapesInSphere(ezPhysicsOverlapResultArray& out_results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const
{
  EZ_PROFILE_SCOPE("QueryShapesInSphere");

  out_results.m_Results.Clear();

  PxQueryFilterData filterData;
  filterData.data = ezPhysX::CreateFilterData(params.m_uiCollisionLayer, params.m_uiIgnoreObjectFilterID);

  // PxQueryFlag::eNO_BLOCK : All hits are reported as touching. Overrides eBLOCK returned from user filters with eTOUCH.
  filterData.flags = PxQueryFlag::ePREFILTER | PxQueryFlag::eNO_BLOCK;

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Static))
  {
    filterData.flags |= PxQueryFlag::eSTATIC;
  }

  if (params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Dynamic))
  {
    filterData.flags |= PxQueryFlag::eDYNAMIC;
  }

  ezPxQueryFilter queryFilter;
  queryFilter.m_bIncludeQueryShapes = params.m_ShapeTypes.IsSet(ezPhysicsShapeType::Query);

  PxSphereGeometry sphere;
  sphere.radius = fSphereRadius;

  PxTransform transform = ezPxConversionUtils::ToTransform(vPosition, ezQuat::MakeIdentity());

  ezArrayPtr<PxOverlapHit> overlapHits = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), PxOverlapHit, 256);
  PxOverlapBuffer overlapHitsBuffer(overlapHits.GetPtr(), overlapHits.GetCount());

  EZ_PX_READ_LOCK(*m_pPxScene);

  m_pPxScene->overlap(sphere, transform, overlapHitsBuffer, filterData, &queryFilter);

  out_results.m_Results.Reserve(overlapHitsBuffer.nbTouches);

  for (ezUInt32 i = 0; i < overlapHitsBuffer.nbTouches; ++i)
  {
    auto& overlapHit = overlapHitsBuffer.touches[i];
    auto& overlapResult = out_results.m_Results.ExpandAndGetRef();

    if (ezComponent* pShapeComponent = ezPxUserData::GetComponent(overlapHit.shape->userData))
    {
      overlapResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
      overlapResult.m_uiObjectFilterID = overlapHit.shape->getQueryFilterData().word2;
    }

    if (ezComponent* pActorComponent = ezPxUserData::GetComponent(overlapHit.actor->userData))
    {
      overlapResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}

void ezPhysXWorldModule::AddStaticCollisionBox(ezGameObject* pObject, ezVec3 vBoxSize)
{
  ezPxStaticActorComponent* pActor = nullptr;
  ezPxStaticActorComponent::CreateComponent(pObject, pActor);

  ezPxShapeBoxComponent* pBox;
  ezPxShapeBoxComponent::CreateComponent(pObject, pBox);
  pBox->SetExtents(vBoxSize);
}

void ezPhysXWorldModule::FreeUserDataAfterSimulationStep()
{
  m_FreeUserData.PushBackRange(m_FreeUserDataAfterSimulationStep);
  m_FreeUserDataAfterSimulationStep.Clear();
}

void ezPhysXWorldModule::StartSimulation(const ezWorldModule::UpdateContext& context)
{
  if (cvar_PhysicsSimulationPause)
    return;

  ezPxDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezPxDynamicActorComponentManager>();
  ezPxTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<ezPxTriggerComponentManager>();
  ezPxQueryShapeActorComponentManager* pQueryShapesManager = GetWorld()->GetComponentManager<ezPxQueryShapeActorComponentManager>();

  EZ_PX_WRITE_LOCK(*m_pPxScene);

  UpdatePVD();

  if (ezPxSettingsComponentManager* pSettingsManager = GetWorld()->GetComponentManager<ezPxSettingsComponentManager>())
  {
    ezPxSettingsComponent* pSettings = pSettingsManager->GetSingletonComponent();
    if (pSettings != nullptr && pSettings->IsModified())
    {
      SetGravity(pSettings->GetObjectGravity(), pSettings->GetCharacterGravity());

      if (pDynamicActorManager != nullptr && pSettings->IsModified(EZ_BIT(2))) // max de-penetration velocity
      {
        pDynamicActorManager->UpdateMaxDepenetrationVelocity(pSettings->GetMaxDepenetrationVelocity());
      }

      m_Settings = pSettings->GetSettings();
      m_AccumulatedTimeSinceUpdate = ezTime::MakeZero();

      m_ScratchMemory.SetCountUninitialized(ezMemoryUtils::AlignSize(m_Settings.m_uiScratchMemorySize, 16u * 1024u));

      pSettings->ResetModified();
    }
  }

  if (pDynamicActorManager != nullptr)
  {
    pDynamicActorManager->UpdateKinematicActors();
  }

  if (pQueryShapesManager != nullptr)
  {
    pQueryShapesManager->UpdateKinematicActors();
  }

  if (pTriggerManager != nullptr)
  {
    pTriggerManager->UpdateKinematicActors();
  }

  UpdateJoints();

  m_SimulateTaskGroupId = ezTaskSystem::StartSingleTask(m_pSimulateTask, ezTaskPriority::EarlyThisFrame);
}

ezColorGammaUB FromARGB(ezUInt32 uiCol)
{
  return ezColorGammaUB(static_cast<ezUInt8>((uiCol & 0x00FF0000) >> 16), static_cast<ezUInt8>((uiCol & 0x0000FF00) >> 8), static_cast<ezUInt8>((uiCol & 0x000000FF)));
}

void ezPhysXWorldModule::FetchResults(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("FetchResults");

  if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld()))
  {
    m_pSimulationEventCallback->m_vMainCameraPosition = pView->GetCamera()->GetPosition();
  }

  {
    EZ_PROFILE_SCOPE("Wait for Simulate Task");
    ezTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  }

  // Nothing to fetch if no simulation step was executed
  if (!m_bSimulationStepExecuted)
    return;

  if (ezPxDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezPxDynamicActorComponentManager>())
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);

    PxU32 numActiveActors = 0;
    PxActor** pActiveActors = m_pPxScene->getActiveActors(numActiveActors);

    if (numActiveActors > 0)
    {
      pDynamicActorManager->UpdateDynamicActors(ezMakeArrayPtr(pActiveActors, numActiveActors));
    }
  }

  // debug draw
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);
    m_pPxScene->setVisualizationParameter(PxVisualizationParameter::eSCALE, cvar_PhysicsDebugDrawEnable ? cvar_PhysicsDebugDrawSize : 0.0f);

    if (cvar_PhysicsDebugDrawEnable)
    {
      // m_pPxScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1);
      // m_pPxScene->setVisualizationParameter(PxVisualizationParameter::eCONTACT_POINT, 1);
      // m_pPxScene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_DYNAMIC, 1);
      m_pPxScene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LIMITS, 1);
      // m_pPxScene->setVisualizationParameter(PxVisualizationParameter::eJOINT_LOCAL_FRAMES, 1);
      // m_pPxScene->setVisualizationParameter(PxVisualizationParameter::eBODY_AXES, 1);

      ezHybridArray<ezDebugRenderer::Line, 64> lines;

      const PxRenderBuffer& rb = m_pPxScene->getRenderBuffer();
      for (PxU32 i = 0; i < rb.getNbLines(); i++)
      {
        const PxDebugLine& line = rb.getLines()[i];

        auto& l = lines.ExpandAndGetRef();
        l.m_start = ezPxConversionUtils::ToVec3(line.pos0);
        l.m_end = ezPxConversionUtils::ToVec3(line.pos1);
        l.m_startColor = FromARGB(line.color0);
        l.m_endColor = FromARGB(line.color1);
      }

      ezDebugRenderer::DrawLines(GetWorld(), lines, ezColor::White);
    }
  }

  HandleSimulationEvents();

  HandleBrokenConstraints();

  HandleTriggerEvents();

  FreeUserDataAfterSimulationStep();
}

void ezPhysXWorldModule::HandleBrokenConstraints()
{
  EZ_PROFILE_SCOPE("HandleBrokenConstraints");

  for (auto pConstraint : m_pSimulationEventCallback->m_BrokenConstraints)
  {
    auto it = m_BreakableJoints.Find(pConstraint);
    if (it.IsValid())
    {
      ezPxJointComponent* pJoint = nullptr;

      if (m_pWorld->TryGetComponent(it.Value(), pJoint))
      {
        ezMsgPhysicsJointBroke msg;
        msg.m_hJointObject = pJoint->GetOwner()->GetHandle();

        pJoint->GetOwner()->PostEventMessage(msg, pJoint, ezTime::MakeZero());
      }

      // it can't break twice
      m_BreakableJoints.Remove(it);
    }
  }

  m_pSimulationEventCallback->m_BrokenConstraints.Clear();
}

void ezPhysXWorldModule::HandleTriggerEvents()
{
  EZ_PROFILE_SCOPE("HandleTriggerEvents");

  for (const auto& te : m_pSimulationEventCallback->m_TriggerEvents)
  {
    ezPxTriggerComponent* pTrigger;
    if (!m_pWorld->TryGetComponent(te.m_hTriggerComponent, pTrigger))
      continue;

    ezComponent* pOther = nullptr;
    if (m_pWorld->TryGetComponent(te.m_hOtherComponent, pOther))
    {
      pTrigger->PostTriggerMessage(pOther, te.m_TriggerState);
    }
  }

  m_pSimulationEventCallback->m_TriggerEvents.Clear();
}

void ezPhysXWorldModule::Simulate()
{
  m_bSimulationStepExecuted = false;
  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  if (m_Settings.m_SteppingMode == ezPxSteppingMode::Variable)
  {
    SimulateStep(tDiff);
  }
  else if (m_Settings.m_SteppingMode == ezPxSteppingMode::Fixed)
  {
    const ezTime tFixedStep = ezTime::Seconds(1.0 / m_Settings.m_fFixedFrameRate);

    m_AccumulatedTimeSinceUpdate += tDiff;
    ezUInt32 uiNumSubSteps = 0;

    while (m_AccumulatedTimeSinceUpdate >= tFixedStep && uiNumSubSteps < m_Settings.m_uiMaxSubSteps)
    {
      SimulateStep(tFixedStep);

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

      SimulateStep(tDeltaTime);

      m_AccumulatedTimeSinceUpdate -= tDeltaTime;
    }
  }
  else
  {
    EZ_REPORT_FAILURE("Invalid stepping mode");
  }

  // not sure where exactly this needs to go
  // m_pCharacterManager->computeInteractions((float)tDiff.GetSeconds());
}

void ezPhysXWorldModule::SimulateStep(ezTime deltaTime)
{
  {
    EZ_PX_WRITE_LOCK(*m_pPxScene);

    EZ_PROFILE_SCOPE("Simulate");
    m_pPxScene->simulate(deltaTime.AsFloatInSeconds(), nullptr, m_ScratchMemory.GetData(), m_ScratchMemory.GetCount());
  }

  {
    EZ_PROFILE_SCOPE("FetchResult");

    // Help executing tasks while we wait for the simulation to finish
    ezTaskSystem::WaitForCondition([&]
      { return m_pPxScene->checkResults(false); });

    EZ_PX_WRITE_LOCK(*m_pPxScene);

    int numFetch = 0;
    while (!m_pPxScene->fetchResults(false))
    {
      ++numFetch;
    }

    EZ_ASSERT_DEBUG(numFetch == 0, "m_pPxScene->fetchResults should have succeeded right away");
  }

  m_bSimulationStepExecuted = true;
}

void ezPhysXWorldModule::UpdatePVD()
{
  // this function doesn't seem to have any effect in the PVD :(

#if EZ_ENABLED(EZ_COMPILE_FOR_DEVELOPMENT)
  physx::PxPvdSceneClient* pvdClient = m_pPxScene->getScenePvdClient();
  if (pvdClient)
  {
    ezStringBuilder camName;

    auto pCamMan = GetWorld()->GetComponentManager<ezCameraComponentManager>();

    if (pCamMan == nullptr)
      return;

    for (auto it = pCamMan->GetComponents(); it.IsValid(); it.Next())
    {
      if (!it->IsActiveAndSimulating())
        continue;

      ezCameraComponent* pCam = it;

      if (pCam->GetUsageHint() == ezCameraUsageHint::EditorView ||
          pCam->GetUsageHint() == ezCameraUsageHint::MainView)
      {
        camName = pCam->GetOwner()->GetName();

        if (camName.IsEmpty())
          camName.Format("Camera {}", ezArgP(pCam));

        const ezVec3 pos = pCam->GetOwner()->GetGlobalPosition();
        const ezVec3 up = pCam->GetOwner()->GetGlobalDirUp();
        const ezVec3 fwd = pCam->GetOwner()->GetGlobalDirForwards();

        pvdClient->updateCamera("Default", ezPxConversionUtils::ToVec3(pos), ezPxConversionUtils::ToVec3(up), ezPxConversionUtils::ToVec3(pos + fwd));
      }
    }
  }
#endif
}

void ezPhysXWorldModule::UpdateJoints()
{
  if (m_RequireUpdate.IsEmpty())
    return;

  ezPxJointComponent* pComponent;
  for (auto& hComponent : m_RequireUpdate)
  {
    if (this->m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      pComponent->ApplySettings();
    }
  }

  m_RequireUpdate.Clear();
}

EZ_STATICLINK_FILE(PhysXPlugin, PhysXPlugin_WorldModule_Implementation_PhysXWorldModule);
