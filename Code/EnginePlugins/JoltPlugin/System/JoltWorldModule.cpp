#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Foundation/Configuration/CVar.h>
#include <Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/GroupFilter.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/PhysicsMaterial.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Actors/JoltStaticActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Character/JoltFpsCharacterControllerComponent.h>
#include <JoltPlugin/Components/JoltSettingsComponent.h>
#include <JoltPlugin/Constraints/JoltConstraintComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Collision/CollideShape.h>
#include <Physics/Collision/Shape/CapsuleShape.h>
#include <Physics/Collision/Shape/SphereShape.h>
#include <Physics/Collision/ShapeCast.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <thread>

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

ezJoltWorldModule::ezJoltWorldModule(ezWorld* pWorld)
  : ezPhysicsWorldModuleInterface(pWorld)
//, m_FreeObjectFilterIDs(ezJolt::GetSingleton()->GetAllocator()) // could use a proxy allocator to bin those
{
  // m_pSimulateTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "", ezMakeDelegate(&ezJoltWorldModule::Simulate, this));
  // m_pSimulateTask->ConfigureTask("Jolt Simulate", ezTaskNesting::Maybe);
}

ezJoltWorldModule::~ezJoltWorldModule() = default;

class ezJoltBodyActivationListener : public JPH::BodyActivationListener
{
public:
  virtual void OnBodyActivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
  {
    const ezJoltUserData* pUserData = reinterpret_cast<const ezJoltUserData*>(inBodyUserData);
    if (ezJoltActorComponent* pActor = ezJoltUserData::GetActorComponent(pUserData))
    {
      m_pActiveActors->Insert(pActor, inBodyID.GetIndexAndSequenceNumber());
    }
  }

  virtual void OnBodyDeactivated(const JPH::BodyID& inBodyID, JPH::uint64 inBodyUserData) override
  {
    const ezJoltUserData* pUserData = reinterpret_cast<const ezJoltUserData*>(inBodyUserData);
    if (ezJoltActorComponent* pActor = ezJoltUserData::GetActorComponent(pUserData))
    {
      m_pActiveActors->Remove(pActor);
    }
  }

  ezMap<ezJoltActorComponent*, ezUInt32>* m_pActiveActors = nullptr;
};

class ezJoltGroupFilter : public JPH::GroupFilter
{
public:
  virtual bool CanCollide(const JPH::CollisionGroup& inGroup1, const JPH::CollisionGroup& inGroup2) const override
  {
    const ezUInt64 id = static_cast<ezUInt64>(inGroup1.GetGroupID()) << 32 | inGroup2.GetGroupID();

    return !m_IgnoreCollisions.Contains(id);
  }

  ezHashSet<ezUInt64> m_IgnoreCollisions;
};


void ezJoltWorldModule::Deinitialize()
{
  m_pSystem.Clear();
  m_pTempAllocator.Clear();

  ezJoltBodyActivationListener* pActivationListener = reinterpret_cast<ezJoltBodyActivationListener*>(m_pActivationListener);
  EZ_DEFAULT_DELETE(pActivationListener);

  ezJoltContactListener* pContactListener = reinterpret_cast<ezJoltContactListener*>(m_pContactListener);
  EZ_DEFAULT_DELETE(pContactListener);

  m_pGroupFilter->Release();
}

void ezJoltWorldModule::Initialize()
{
  UpdateSettingsCfg();

  // ensure the first element is reserved for 'invalid' objects
  m_AllocatedUserData.SetCount(1);

  // TODO: all this should be in OnSimulationStarted()

  // TODO: temp alloc size ???
  m_pTempAllocator = EZ_NEW(ezFoundation::GetAlignedAllocator(), JPH::TempAllocatorImpl, 100 * 1024 * 1024);

  // TODO: simulation limits ??
  const uint32_t cMaxBodies = 10000;
  const uint32_t cNumBodyMutexes = 0;
  const uint32_t cMaxBodyPairs = 10000;
  const uint32_t cMaxContactConstraints = 50000;

  m_pSystem = EZ_NEW(ezFoundation::GetAlignedAllocator(), JPH::PhysicsSystem);
  m_pSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, m_ObjectToBroadphase, ezJoltCollisionFiltering::BroadphaseFilter, ezJoltCollisionFiltering::ObjectLayerFilter);

  {
    ezJoltBodyActivationListener* pListener = EZ_DEFAULT_NEW(ezJoltBodyActivationListener);
    m_pActivationListener = pListener;
    pListener->m_pActiveActors = &m_ActiveActors;
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
}

void ezJoltWorldModule::OnSimulationStarted()
{
  {
    auto startSimDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltWorldModule::StartSimulation, this);
    startSimDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    startSimDesc.m_bOnlyUpdateWhenSimulating = true;
    // Start physics simulation as late as possible in the first synchronous phase
    // so all kinematic objects have a chance to update their transform before.
    startSimDesc.m_fPriority = -100000.0f;

    RegisterUpdateFunction(startSimDesc);
  }

  {
    auto fetchResultsDesc = EZ_CREATE_MODULE_UPDATE_FUNCTION_DESC(ezJoltWorldModule::FetchResults, this);
    fetchResultsDesc.m_Phase = ezWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    fetchResultsDesc.m_bOnlyUpdateWhenSimulating = true;
    // Fetch results as early as possible after async phase.
    fetchResultsDesc.m_fPriority = 100000.0f;

    RegisterUpdateFunction(fetchResultsDesc);
  }

  ezJoltCollisionFiltering::LoadCollisionFilters();

  UpdateSettingsCfg();
  ApplySettingsCfg();

  m_AccumulatedTimeSinceUpdate.SetZero();
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

void ezJoltWorldModule::DeleteObjectFilterID(ezUInt32& uiObjectFilterID)
{
  if (uiObjectFilterID == ezInvalidIndex)
    return;

  m_FreeObjectFilterIDs.PushBack(uiObjectFilterID);

  uiObjectFilterID = ezInvalidIndex;
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

void ezJoltWorldModule::DeallocateUserData(ezUInt32& uiUserDataId)
{
  if (uiUserDataId == ezInvalidIndex)
    return;

  m_AllocatedUserData[uiUserDataId].Invalidate();

  m_FreeUserDataAfterSimulationStep.PushBack(uiUserDataId);

  uiUserDataId = ezInvalidIndex;
}

const ezJoltUserData& ezJoltWorldModule::GetUserData(ezUInt32 uiUserDataId) const
{
  EZ_ASSERT_DEBUG(uiUserDataId != ezInvalidIndex, "Invalid ezJoltUserData ID");

  return m_AllocatedUserData[uiUserDataId];
}

void ezJoltWorldModule::SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity)
{
  m_Settings.m_vObjectGravity = objectGravity;
  m_Settings.m_vCharacterGravity = characterGravity;

  if (m_pSystem)
  {
    m_pSystem->SetGravity(ezJoltConversionUtils::ToVec3(m_Settings.m_vObjectGravity));
  }
}

void FillCastResult(ezPhysicsCastResult& result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const JPH::BodyID& bodyId, const JPH::SubShapeID& subShapeId, const JPH::BodyLockInterface& lockInterface, const JPH::BodyInterface& bodyInterface, const ezJoltWorldModule* pModule)
{
  JPH::BodyLockRead bodyLock(lockInterface, bodyId);
  const auto& body = bodyLock.GetBody();
  result.m_vNormal = ezJoltConversionUtils::ToVec3(body.GetWorldSpaceSurfaceNormal(subShapeId, ezJoltConversionUtils::ToVec3(result.m_vPosition)));
  result.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();

  if (ezComponent* pShapeComponent = ezJoltUserData::GetShapeComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(subShapeId))))
  {
    result.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
  }

  if (ezComponent* pActorComponent = ezJoltUserData::GetActorComponent(reinterpret_cast<const void*>(body.GetUserData())))
  {
    result.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
  }

  if (const ezJoltMaterial* pMaterial = static_cast<const ezJoltMaterial*>(bodyInterface.GetMaterial(bodyId, subShapeId)))
  {
    result.m_hSurface = pMaterial->m_pSurface->GetResourceHandle();
  }

  const size_t uiBodyId = bodyId.GetIndexAndSequenceNumber();
  const size_t uiShapeId = subShapeId.GetValue();
  result.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
  result.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);
}

class ezRayCastCollector : public JPH::CastRayCollector
{
public:
  JPH::RayCastResult m_Result;
  bool m_bAnyHit = false;
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::RayCastResult& inResult) override
  {
    if (inResult.mFraction < m_Result.mFraction)
    {
      m_Result = inResult;
      m_bFoundAny = true;

      if (m_bAnyHit)
      {
        ForceEarlyOut();
      }
    }
  }
};

bool ezJoltWorldModule::Raycast(ezPhysicsCastResult& out_Result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection /*= ezPhysicsHitCollection::Closest*/) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RayCast ray;
  ray.mOrigin = ezJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = ezJoltConversionUtils::ToVec3(vDir * fDistance);

  ezRayCastCollector collector;
  collector.m_bAnyHit = collection == ezPhysicsHitCollection::Any;

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  ezJoltObjectLayerFilter objectFilter;
  objectFilter.m_uiCollisionLayer = params.m_uiCollisionLayer;

  if (params.m_bIgnoreInitialOverlap)
  {
    JPH::RayCastSettings opt;
    opt.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
    opt.mTreatConvexAsSolid = false;

    query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

    if (collector.m_bFoundAny == false)
      return false;
  }
  else
  {
    if (!query.CastRay(ray, collector.m_Result, broadphaseFilter, objectFilter, bodyFilter))
      return false;
  }

  out_Result.m_fDistance = collector.m_Result.mFraction * fDistance;
  out_Result.m_vPosition = vStart + fDistance * collector.m_Result.mFraction * vDir;

  FillCastResult(out_Result, vStart, vDir, fDistance, collector.m_Result.mBodyID, collector.m_Result.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class ezRayCastCollectorAll : public JPH::CastRayCollector
{
public:
  ezArrayPtr<JPH::RayCastResult> m_Results;
  ezUInt32 m_uiFound = 0;

  virtual void AddHit(const JPH::RayCastResult& inResult) override
  {
    m_Results[m_uiFound] = inResult;
    ++m_uiFound;
  }
};

bool ezJoltWorldModule::RaycastAll(ezPhysicsCastResultArray& out_Results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RayCast ray;
  ray.mOrigin = ezJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = ezJoltConversionUtils::ToVec3(vDir * fDistance);

  ezRayCastCollectorAll collector;
  collector.m_Results = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), JPH::RayCastResult, 256);

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  ezJoltObjectLayerFilter objectFilter;
  objectFilter.m_uiCollisionLayer = params.m_uiCollisionLayer;

  JPH::RayCastSettings opt;
  opt.mBackFaceMode = params.m_bIgnoreInitialOverlap ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
  opt.mTreatConvexAsSolid = !params.m_bIgnoreInitialOverlap;

  query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

  if (collector.m_uiFound == 0)
    return false;

  out_Results.m_Results.SetCount(collector.m_uiFound);

  for (ezUInt32 i = 0; i < collector.m_uiFound; ++i)
  {
    out_Results.m_Results[i].m_fDistance = collector.m_Results[i].mFraction * fDistance;
    out_Results.m_Results[i].m_vPosition = vStart + fDistance * collector.m_Results[i].mFraction * vDir;

    FillCastResult(out_Results.m_Results[i], vStart, vDir, fDistance, collector.m_Results[i].mBodyID, collector.m_Results[i].mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);
  }

  return true;
}

class ezJoltShapeCastCollector : public JPH::CastShapeCollector
{
public:
  JPH::ShapeCastResult m_Result;
  bool m_bFoundAny = false;
  bool m_bAnyHit = false;

  virtual void AddHit(const JPH::ShapeCastResult& inResult) override
  {
    if (inResult.mIsBackFaceHit)
      return;

    if (inResult.mFraction >= GetEarlyOutFraction())
      return;

    m_bFoundAny = true;
    m_Result = inResult;

    UpdateEarlyOutFraction(inResult.mFraction);

    if (m_bAnyHit)
      ForceEarlyOut();
  }
};

bool ezJoltWorldModule::SweepTestSphere(ezPhysicsCastResult& out_Result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return SweepTest(out_Result, shape, JPH::Mat44::sTranslation(ezJoltConversionUtils::ToVec3(vStart)), vDir, fDistance, params, collection);
}

bool ezJoltWorldModule::SweepTestBox(ezPhysicsCastResult& out_Result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  const JPH::BoxShape shape(ezJoltConversionUtils::ToVec3(vBoxExtends * 0.5f));

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(transform.m_qRotation), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_Result, shape, trans, vDir, fDistance, params, collection);
}

bool ezJoltWorldModule::SweepTestCapsule(ezPhysicsCastResult& out_Result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  if (fCapsuleRadius <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fCapsuleHeight * 0.5f, fCapsuleRadius);

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qRot), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_Result, shape, trans, vDir, fDistance, params, collection);
}

bool ezJoltWorldModule::SweepTest(ezPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  ezJoltObjectLayerFilter objectFilter;
  objectFilter.m_uiCollisionLayer = params.m_uiCollisionLayer;

  JPH::ShapeCast cast(&shape, JPH::Vec3(1, 1, 1), transform, ezJoltConversionUtils::ToVec3(vDir * fDistance));

  ezJoltShapeCastCollector collector;
  collector.m_bAnyHit = collection == ezPhysicsHitCollection::Any;

  query.CastShape(cast, {}, collector, broadphaseFilter, objectFilter, bodyFilter);

  if (!collector.m_bFoundAny)
    return false;

  const auto& res = collector.m_Result;

  out_Result.m_fDistance = res.mFraction * fDistance;
  out_Result.m_vPosition = ezJoltConversionUtils::ToVec3(res.mContactPointOn2);

  FillCastResult(out_Result, ezJoltConversionUtils::ToVec3(transform.GetTranslation()), vDir, fDistance, res.mBodyID2, res.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class ezJoltShapeCollectorAny : public JPH::CollideShapeCollector
{
public:
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::CollideShapeResult& inResult) override
  {
    m_bFoundAny = true;
    ForceEarlyOut();
  }
};

class ezJoltShapeCollectorAll : public JPH::CollideShapeCollector
{
public:
  ezHybridArray<JPH::CollideShapeResult, 32> m_Results;

  virtual void AddHit(const JPH::CollideShapeResult& inResult) override
  {
    m_Results.PushBack(inResult);

    if (m_Results.GetCount() >= 256)
    {
      ForceEarlyOut();
    }
  }
};

bool ezJoltWorldModule::OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return OverlapTest(shape, JPH::Mat44::sTranslation(ezJoltConversionUtils::ToVec3(vPosition)), params);
}

bool ezJoltWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  if (fCapsuleRadius <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fCapsuleHeight * 0.5f, fCapsuleRadius);

  ezQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::Degree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qRot), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool ezJoltWorldModule::OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const ezPhysicsQueryParameters& params) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);

  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  ezJoltObjectLayerFilter objectFilter;
  objectFilter.m_uiCollisionLayer = params.m_uiCollisionLayer;

  ezJoltShapeCollectorAny collector;
  query.CollideShape(&shape, JPH::Vec3(1, 1, 1), transform, {}, collector, broadphaseFilter, objectFilter, bodyFilter);

  return collector.m_bFoundAny;
}

void ezJoltWorldModule::QueryShapesInSphere(ezPhysicsOverlapResultArray& out_Results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const
{
  out_Results.m_Results.Clear();

  if (fSphereRadius <= 0.0f)
    return;

  const JPH::SphereShape shape(fSphereRadius);
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);

  ezJoltObjectLayerFilter objectFilter;
  objectFilter.m_uiCollisionLayer = params.m_uiCollisionLayer;

  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  ezJoltShapeCollectorAll collector;
  query.CollideShape(&shape, JPH::Vec3(1, 1, 1), JPH::Mat44::sTranslation(ezJoltConversionUtils::ToVec3(vPosition)), {}, collector, broadphaseFilter, objectFilter, bodyFilter);

  out_Results.m_Results.SetCount(collector.m_Results.GetCount());

  auto& lockInterface = m_pSystem->GetBodyLockInterfaceNoLock();

  for (ezUInt32 i = 0; i < collector.m_Results.GetCount(); ++i)
  {
    auto& overlapResult = out_Results.m_Results[i];
    auto& overlapHit = collector.m_Results[i];

    JPH::BodyLockRead bodyLock(lockInterface, overlapHit.mBodyID2);
    const auto& body = bodyLock.GetBody();

    overlapResult.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();

    if (ezComponent* pShapeComponent = ezJoltUserData::GetShapeComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(overlapHit.mSubShapeID2))))
    {
      overlapResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
    }

    if (ezComponent* pActorComponent = ezJoltUserData::GetActorComponent(reinterpret_cast<const void*>(body.GetUserData())))
    {
      overlapResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}

void ezJoltWorldModule::AddStaticCollisionBox(ezGameObject* pObject, ezVec3 boxSize)
{
  ezJoltStaticActorComponent* pActor = nullptr;
  ezJoltStaticActorComponent::CreateComponent(pObject, pActor);

  ezJoltShapeBoxComponent* pBox;
  ezJoltShapeBoxComponent::CreateComponent(pObject, pBox);
  pBox->SetHalfExtents(boxSize * 0.5f);
}

void ezJoltWorldModule::QueueBodyToAdd(JPH::Body* pBody)
{
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
      m_pSystem->GetBodyInterface().AddBodiesFinalize(pIDs, uiCount, pHandle, JPH::EActivation::Activate);

      uiStartIdx += uiCount;
    }

    m_BodiesToAdd.Clear();
  }

  if (m_uiBodiesAddedSinceOptimize > 128)
  {
    // TODO: not clear whether this could be multi-threaded or done more efficiently somehow
    m_pSystem->OptimizeBroadPhase();
    m_uiBodiesAddedSinceOptimize = 0;
  }

  ezJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezJoltDynamicActorComponentManager>();
  ezJoltFpsCharacterControllerComponentManager* pCharacterManager = GetWorld()->GetComponentManager<ezJoltFpsCharacterControllerComponentManager>();
  ezJoltTriggerComponentManager* pTriggerManager = GetWorld()->GetComponentManager<ezJoltTriggerComponentManager>();
  ezJoltQueryShapeActorComponentManager* pQueryShapesManager = GetWorld()->GetComponentManager<ezJoltQueryShapeActorComponentManager>();

  UpdateSettingsCfg();

  if (pDynamicActorManager != nullptr)
  {
    pDynamicActorManager->UpdateKinematicActors();
  }

  if (pQueryShapesManager != nullptr)
  {
    pQueryShapesManager->UpdateMovingQueryShapes();
  }

  if (pTriggerManager != nullptr)
  {
    pTriggerManager->UpdateMovingTriggers();
  }

  UpdateConstraints();

  Simulate();

#ifdef JPH_DEBUG_RENDERER
  if (cvar_JoltDebugDrawConstraints)
    m_pSystem->DrawConstraints(ezJoltCore::s_pDebugRenderer.Borrow());

  if (cvar_JoltDebugDrawConstraintLimits)
    m_pSystem->DrawConstraintLimits(ezJoltCore::s_pDebugRenderer.Borrow());

  if (cvar_JoltDebugDrawConstraintFrames)
    m_pSystem->DrawConstraintReferenceFrame(ezJoltCore::s_pDebugRenderer.Borrow());

  if (cvar_JoltDebugDrawBodies)
  {
    JPH::BodyManager::DrawSettings opt;
    opt.mDrawShape = true;
    opt.mDrawShapeWireframe = true;
    m_pSystem->DrawBodies(opt, ezJoltCore::s_pDebugRenderer.Borrow());
  }
#endif

  ezJoltCore::DebugDraw(GetWorld());
  //  m_SimulateTaskGroupId = ezTaskSystem::StartSingleTask(m_pSimulateTask, ezTaskPriority::EarlyThisFrame);
}


void ezJoltWorldModule::FetchResults(const ezWorldModule::UpdateContext& context)
{
  EZ_PROFILE_SCOPE("FetchResults");

  if (ezView* pView = ezRenderWorld::GetViewByUsageHint(ezCameraUsageHint::MainView, ezCameraUsageHint::EditorView, GetWorld()))
  {
    reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_Events.m_vMainCameraPosition = pView->GetCamera()->GetPosition();
  }

  //  {
  //    EZ_PROFILE_SCOPE("Wait for Simulate Task");
  //    ezTaskSystem::WaitForGroup(m_SimulateTaskGroupId);
  //  }
  //
  //  // Nothing to fetch if no simulation step was executed
  //  if (!m_bSimulationStepExecuted)
  //    return;
  //

  if (ezJoltDynamicActorComponentManager* pDynamicActorManager = GetWorld()->GetComponentManager<ezJoltDynamicActorComponentManager>())
  {
    pDynamicActorManager->UpdateDynamicActors();
  }

  reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_Events.SpawnPhysicsImpactReactions();
  reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_Events.UpdatePhysicsSlideReactions();
  reinterpret_cast<ezJoltContactListener*>(m_pContactListener)->m_Events.UpdatePhysicsRollReactions();

  //  HandleBrokenConstraints();

  FreeUserDataAfterSimulationStep();
}

// void ezJoltWorldModule::HandleBrokenConstraints()
//{
//   EZ_PROFILE_SCOPE("HandleBrokenConstraints");
//
//   for (auto pConstraint : m_pSimulationEventCallback->m_BrokenConstraints)
//   {
//     auto it = m_BreakableJoints.Find(pConstraint);
//     if (it.IsValid())
//     {
//       ezJoltConstraintComponent* pJoint = nullptr;
//
//       if (m_pWorld->TryGetComponent(it.Value(), pJoint))
//       {
//         ezMsgPhysicsJointBroke msg;
//         msg.m_hJointObject = pJoint->GetOwner()->GetHandle();
//
//         pJoint->GetOwner()->PostEventMessage(msg, pJoint, ezTime::Zero());
//       }
//
//       // it can't break twice
//       m_BreakableJoints.Remove(it);
//     }
//   }
//
//   m_pSimulationEventCallback->m_BrokenConstraints.Clear();
// }

void ezJoltWorldModule::Simulate()
{
  // m_bSimulationStepExecuted = false;
  const ezTime tDiff = GetWorld()->GetClock().GetTimeDiff();

  if (m_Settings.m_SteppingMode == ezJoltSteppingMode::Variable)
  {
    SimulateStep(tDiff);
  }
  else if (m_Settings.m_SteppingMode == ezJoltSteppingMode::Fixed)
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
  else if (m_Settings.m_SteppingMode == ezJoltSteppingMode::SemiFixed)
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
}

void ezJoltWorldModule::SimulateStep(ezTime deltaTime)
{
  {
    EZ_PROFILE_SCOPE("Simulate");

    const int cCollisionSteps = 1;
    const int cIntegrationSubSteps = 1;

    m_pSystem->Update(deltaTime.AsFloatInSeconds(), cCollisionSteps, cIntegrationSubSteps, m_pTempAllocator.Borrow(), ezJoltCore::GetJoltJobSystem());
  }
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
