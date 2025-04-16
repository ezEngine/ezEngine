#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <Physics/Collision/CollisionCollectorImpl.h>

void FillCastResult(ezPhysicsCastResult& ref_result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const JPH::BodyID& bodyId, const JPH::SubShapeID& subShapeId, const JPH::BodyLockInterface& lockInterface, const JPH::BodyInterface& bodyInterface, const ezJoltWorldModule* pModule)
{
  JPH::BodyLockRead bodyLock(lockInterface, bodyId);
  EZ_ASSERT_DEBUG(bodyLock.Succeeded(), "Failed to get body lock");
  const auto& body = bodyLock.GetBody();
  ref_result.m_vNormal = ezJoltConversionUtils::ToVec3(body.GetWorldSpaceSurfaceNormal(subShapeId, ezJoltConversionUtils::ToVec3(ref_result.m_vPosition)));
  ref_result.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();

  if (ezComponent* pShapeComponent = ezJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(subShapeId))))
  {
    ref_result.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
  }

  if (ezComponent* pActorComponent = ezJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
  {
    ref_result.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
  }

  if (const ezJoltMaterial* pMaterial = static_cast<const ezJoltMaterial*>(bodyInterface.GetMaterial(bodyId, subShapeId)))
  {
    if (pMaterial->m_pSurface)
    {
      ref_result.m_hSurface = pMaterial->m_pSurface->GetResourceHandle();
    }
  }

  const size_t uiBodyId = bodyId.GetIndexAndSequenceNumber();
  const size_t uiShapeId = subShapeId.GetValue();
  ref_result.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
  ref_result.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);
}

class ezRayCastCollector : public JPH::CastRayCollector
{
public:
  JPH::RayCastResult m_Result;
  bool m_bAnyHit = false;
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    if (result.mFraction < m_Result.mFraction)
    {
      m_Result = result;
      m_bFoundAny = true;

      if (m_bAnyHit)
      {
        ForceEarlyOut();
      }
    }
  }
};

bool ezJoltWorldModule::Raycast(ezPhysicsCastResult& out_result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection /*= ezPhysicsHitCollection::Closest*/) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
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
    opt.mBackFaceModeTriangles = JPH::EBackFaceMode::IgnoreBackFaces;
    opt.mBackFaceModeConvex = JPH::EBackFaceMode::IgnoreBackFaces;
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

  out_result.m_fDistance = collector.m_Result.mFraction * fDistance;
  out_result.m_vPosition = vStart + fDistance * collector.m_Result.mFraction * vDir;

  FillCastResult(out_result, vStart, vDir, fDistance, collector.m_Result.mBodyID, collector.m_Result.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class ezRayCastCollectorAll : public JPH::CastRayCollector
{
public:
  ezArrayPtr<JPH::RayCastResult> m_Results;
  ezUInt32 m_uiFound = 0;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    m_Results[m_uiFound] = result;
    ++m_uiFound;
  }
};

bool ezJoltWorldModule::RaycastAll(ezPhysicsCastResultArray& out_results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
  ray.mOrigin = ezJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = ezJoltConversionUtils::ToVec3(vDir * fDistance);

  ezRayCastCollectorAll collector;
  collector.m_Results = EZ_NEW_ARRAY(ezFrameAllocator::GetCurrentAllocator(), JPH::RayCastResult, 256);

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  JPH::RayCastSettings opt;
  opt.mBackFaceModeTriangles = params.m_bIgnoreInitialOverlap ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
  opt.mBackFaceModeConvex = opt.mBackFaceModeTriangles;
  opt.mTreatConvexAsSolid = !params.m_bIgnoreInitialOverlap;

  query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

  if (collector.m_uiFound == 0)
    return false;

  out_results.m_Results.SetCount(collector.m_uiFound);

  for (ezUInt32 i = 0; i < collector.m_uiFound; ++i)
  {
    out_results.m_Results[i].m_fDistance = collector.m_Results[i].mFraction * fDistance;
    out_results.m_Results[i].m_vPosition = vStart + fDistance * collector.m_Results[i].mFraction * vDir;

    FillCastResult(out_results.m_Results[i], vStart, vDir, fDistance, collector.m_Results[i].mBodyID, collector.m_Results[i].mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);
  }

  return true;
}

class ezJoltShapeCastCollector : public JPH::CastShapeCollector
{
public:
  JPH::ShapeCastResult m_Result;
  bool m_bFoundAny = false;
  bool m_bAnyHit = false;

  virtual void AddHit(const JPH::ShapeCastResult& result) override
  {
    if (result.mIsBackFaceHit)
      return;

    if (result.mFraction >= GetEarlyOutFraction())
      return;

    m_bFoundAny = true;
    m_Result = result;

    UpdateEarlyOutFraction(result.mFraction);

    if (m_bAnyHit)
      ForceEarlyOut();
  }
};

bool ezJoltWorldModule::SweepTestSphere(ezPhysicsCastResult& out_result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return SweepTest(out_result, shape, JPH::Mat44::sTranslation(ezJoltConversionUtils::ToVec3(vStart)), vDir, fDistance, params, collection);
}

bool ezJoltWorldModule::SweepTestBox(ezPhysicsCastResult& out_result, const ezVec3& vBoxExtents, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  if (vBoxExtents.x <= 0.0f || vBoxExtents.y <= 0.0f || vBoxExtents.z <= 0.0f)
    return false;

  const JPH::BoxShape shape(ezJoltConversionUtils::ToVec3(vBoxExtents.CompMul(transform.m_vScale.Abs()) * 0.5f));

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(transform.m_qRotation), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool ezJoltWorldModule::SweepTestCapsule(ezPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  const ezVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCapsuleHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCapsuleRadius * ezMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qRot), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool ezJoltWorldModule::SweepTestCylinder(ezPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  const ezVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCylinderHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCylinderRadius * ezMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CylinderShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qRot), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool ezJoltWorldModule::SweepTest(ezPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  JPH::RShapeCast cast(&shape, JPH::RVec3::sOne(), transform, ezJoltConversionUtils::ToVec3(vDir * fDistance));

  ezJoltShapeCastCollector collector;
  collector.m_bAnyHit = collection == ezPhysicsHitCollection::Any;

  query.CastShape(cast, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

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

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_bFoundAny = true;
    ForceEarlyOut();
  }
};

class ezJoltShapeCollectorAll : public JPH::CollideShapeCollector
{
public:
  ezHybridArray<JPH::CollideShapeResult, 32, ezAlignedAllocatorWrapper> m_Results;

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_Results.PushBack(result);

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

bool ezJoltWorldModule::OverlapTestBox(const ezVec3& vBoxExtents, const ezVec3& vPosition, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  if (vBoxExtents.x <= 0.0f || vBoxExtents.y <= 0.0f || vBoxExtents.z <= 0.0f)
    return false;

  const JPH::BoxShape shape(ezJoltConversionUtils::ToVec3(vBoxExtents.CompMul(transform.m_vScale.Abs()) * 0.5f));

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(transform.m_qRotation),
                                                            ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool ezJoltWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  const ezVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCapsuleHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCapsuleRadius * ezMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qRot), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool ezJoltWorldModule::OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  const ezVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCylinderHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCylinderRadius * ezMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return false;

  const JPH::CylinderShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90.0f));

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
  query.CollideShape(&shape, JPH::RVec3::sOne(), transform, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  return collector.m_bFoundAny;
}

void ezJoltWorldModule::QueryShapesInSphere(ezPhysicsOverlapResultArray& out_results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  if (fSphereRadius <= 0.0f)
    return;

  const JPH::SphereShape shape(fSphereRadius);
  const JPH::Mat44 trans = JPH::Mat44::sTranslation(ezJoltConversionUtils::ToVec3(vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void ezJoltWorldModule::QueryShapesInBox(ezPhysicsOverlapResultArray& out_results, const ezVec3& vBoxExtents, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  if (vBoxExtents.x <= 0.0f || vBoxExtents.y <= 0.0f || vBoxExtents.z <= 0.0f)
    return;

  const JPH::BoxShape shape(ezJoltConversionUtils::ToVec3(vBoxExtents.CompMul(transform.m_vScale.Abs()) * 0.5f));
  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(transform.m_qRotation),
                                                            ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void ezJoltWorldModule::QueryShapesInCapsule(ezPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  const ezVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCapsuleHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCapsuleRadius * ezMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return;

  const JPH::CapsuleShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qRot), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void ezJoltWorldModule::QueryShapesInCylinder(ezPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylindereHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  const ezVec3 vScaleAbs = transform.m_vScale.Abs();
  const float fHeightTransformed = fCylindereHeight * vScaleAbs.z;
  const float fRadiusTransformed = fCylinderRadius * ezMath::Max(vScaleAbs.x, vScaleAbs.y);

  if (fRadiusTransformed <= 0.0f || fHeightTransformed <= 0.0f)
    return;

  const JPH::CylinderShape shape(fHeightTransformed * 0.5f, fRadiusTransformed);

  ezQuat qFixRot = ezQuat::MakeFromAxisAndAngle(ezVec3(1, 0, 0), ezAngle::MakeFromDegree(90.0f));

  ezQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(ezJoltConversionUtils::ToQuat(qRot), ezJoltConversionUtils::ToVec3(transform.m_vPosition));

  QueryShapes(out_results, shape, trans, params);
}

void ezJoltWorldModule::QueryShapes(ezPhysicsOverlapResultArray& out_results, const JPH::Shape& shape, const JPH::Mat44& transform, const ezPhysicsQueryParameters& params) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);
  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  ezJoltShapeCollectorAll collector;

  query.CollideShape(&shape, JPH::RVec3::sOne(), transform, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  out_results.m_Results.SetCount(collector.m_Results.GetCount());

  auto& lockInterface = m_pSystem->GetBodyLockInterfaceNoLock();

  for (ezUInt32 i = 0; i < collector.m_Results.GetCount(); ++i)
  {
    auto& overlapResult = out_results.m_Results[i];
    auto& overlapHit = collector.m_Results[i];

    JPH::BodyLockRead bodyLock(lockInterface, overlapHit.mBodyID2);
    EZ_ASSERT_DEBUG(bodyLock.Succeeded(), "Failed to get body lock");
    const auto& body = bodyLock.GetBody();

    overlapResult.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();
    overlapResult.m_vCenterPosition = ezJoltConversionUtils::ToVec3(body.GetCenterOfMassPosition());

    const size_t uiBodyId = body.GetID().GetIndexAndSequenceNumber();
    const size_t uiShapeId = overlapHit.mSubShapeID2.GetValue();
    overlapResult.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
    overlapResult.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);

    if (ezComponent* pShapeComponent = ezJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(overlapHit.mSubShapeID2))))
    {
      overlapResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
    }

    if (ezComponent* pActorComponent = ezJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
    {
      overlapResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}

void ezJoltWorldModule::QueryGeometryInBox(const ezPhysicsQueryParameters& params, ezBoundingBox box, ezDynamicArray<ezNavmeshTriangle>& out_triangles) const
{
  JPH::AABox aabb;
  aabb.mMin = ezJoltConversionUtils::ToVec3(box.m_vMin);
  aabb.mMax = ezJoltConversionUtils::ToVec3(box.m_vMax);

  JPH::AllHitCollisionCollector<JPH::TransformedShapeCollector> collector;

  ezJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  ezJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);
  ezJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  m_pSystem->GetNarrowPhaseQuery().CollectTransformedShapes(aabb, collector, broadphaseFilter, objectFilter, bodyFilter);

  const int cMaxTriangles = 128;

  ezStaticArray<ezVec3, cMaxTriangles * 3> positionsTmp;
  positionsTmp.SetCountUninitialized(cMaxTriangles * 3);

  ezStaticArray<const JPH::PhysicsMaterial*, cMaxTriangles> materialsTmp;
  materialsTmp.SetCountUninitialized(cMaxTriangles);

  for (const JPH::TransformedShape& ts : collector.mHits)
  {
    JPH::Shape::GetTrianglesContext ctx;
    ts.GetTrianglesStart(ctx, aabb, JPH::Vec3::sZero());

    while (true)
    {
      const int triCount = ts.GetTrianglesNext(ctx, cMaxTriangles, reinterpret_cast<JPH::Float3*>(positionsTmp.GetData()), materialsTmp.GetData());

      if (triCount == 0)
        break;

      out_triangles.Reserve(out_triangles.GetCount() + triCount);

      for (int i = 0; i < triCount; ++i)
      {
        const ezJoltMaterial* pMat = static_cast<const ezJoltMaterial*>(materialsTmp[i]);

        auto& tri = out_triangles.ExpandAndGetRef();
        tri.m_pSurface = pMat ? pMat->m_pSurface : nullptr;
        tri.m_Vertices[0] = positionsTmp[i * 3 + 0];
        tri.m_Vertices[1] = positionsTmp[i * 3 + 1];
        tri.m_Vertices[2] = positionsTmp[i * 3 + 2];
      }
    }
  }
}
