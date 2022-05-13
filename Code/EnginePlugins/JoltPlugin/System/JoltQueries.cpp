#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

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
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

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
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

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
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

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
  ezHybridArray<JPH::CollideShapeResult, 32, ezAlignedAllocatorWrapper> m_Results;

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
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

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
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);
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
