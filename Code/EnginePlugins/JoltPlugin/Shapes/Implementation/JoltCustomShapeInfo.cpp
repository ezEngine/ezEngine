#include <JoltPlugin/JoltPluginPCH.h>

#include <Jolt/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>

using namespace JPH;

const JPH::PhysicsMaterial* ezJoltCustomShapeInfo::GetMaterial(const SubShapeID& subShapeID) const
{
  if (!m_CustomMaterials.IsEmpty())
  {
    if (mInnerShape->GetSubType() == EShapeSubType::Mesh)
    {
      const JPH::MeshShape* pMeshShape = static_cast<const JPH::MeshShape*>(mInnerShape.GetPtr());

      return m_CustomMaterials[pMeshShape->GetMaterialIndex(subShapeID)];
    }

    return m_CustomMaterials[0];
  }

  return mInnerShape->GetMaterial(subShapeID);
}

JPH::uint64 ezJoltCustomShapeInfo::GetSubShapeUserData(const SubShapeID& subShapeID) const
{
  return GetUserData();
}

JPH::MassProperties ezJoltCustomShapeInfo::GetMassProperties() const
{
  MassProperties p = mInnerShape->GetMassProperties();

  p.mMass *= m_fDensity;
  p.mInertia *= m_fDensity;
  p.mInertia(3, 3) = 1.0f;

  return p;
}

JPH::Vec3 ezJoltCustomShapeInfo::GetCenterOfMass() const
{
  return mInnerShape->GetCenterOfMass();
}

//////////////////////////////////////////////////////////////////////////

JPH::AABox ezJoltCustomShapeInfo::GetLocalBounds() const
{
  return mInnerShape->GetLocalBounds();
}

float ezJoltCustomShapeInfo::GetInnerRadius() const
{
  return mInnerShape->GetInnerRadius();
}

JPH::Vec3 ezJoltCustomShapeInfo::GetSurfaceNormal(const SubShapeID& subShapeID, Vec3Arg inLocalSurfacePosition) const
{
  return mInnerShape->GetSurfaceNormal(subShapeID, inLocalSurfacePosition);
}

void ezJoltCustomShapeInfo::GetSubmergedVolume(Mat44Arg centerOfMassTransform, Vec3Arg inScale, const Plane& surface, float& out_fTotalVolume, float& out_fSubmergedVolume, Vec3& out_centerOfBuoyancy
#ifdef JPH_DEBUG_RENDERER // Not using JPH_IF_DEBUG_RENDERER for Doxygen
  ,
  JPH::RVec3Arg inBaseOffset
#endif
) const
{
  mInnerShape->GetSubmergedVolume(centerOfMassTransform, inScale, surface, out_fTotalVolume, out_fSubmergedVolume, out_centerOfBuoyancy
#ifdef JPH_DEBUG_RENDERER // Not using JPH_IF_DEBUG_RENDERER for Doxygen
    ,
    inBaseOffset
#endif
  );
}

void ezJoltCustomShapeInfo::Draw(DebugRenderer* pInRenderer, Mat44Arg centerOfMassTransform, Vec3Arg inScale, ColorArg inColor, bool bInUseMaterialColors, bool bInDrawWireframe) const
{
  mInnerShape->Draw(pInRenderer, centerOfMassTransform, inScale, inColor, bInUseMaterialColors, bInDrawWireframe);
}

bool ezJoltCustomShapeInfo::CastRay(const RayCast& ray, const SubShapeIDCreator& subShapeIDCreator, RayCastResult& ref_hit) const
{
  return mInnerShape->CastRay(ray, subShapeIDCreator, ref_hit);
}

void ezJoltCustomShapeInfo::CastRay(const JPH::RayCast& ray, const JPH::RayCastSettings& rayCastSettings, const JPH::SubShapeIDCreator& subShapeIDCreator, JPH::CastRayCollector& ref_collector, const JPH::ShapeFilter& shapeFilter) const
{
  return mInnerShape->CastRay(ray, rayCastSettings, subShapeIDCreator, ref_collector, shapeFilter);
}

void ezJoltCustomShapeInfo::CollidePoint(JPH::Vec3Arg inPoint, const JPH::SubShapeIDCreator& subShapeIDCreator, JPH::CollidePointCollector& ref_collector, const JPH::ShapeFilter& shapeFilter) const
{
  mInnerShape->CollidePoint(inPoint, subShapeIDCreator, ref_collector, shapeFilter);
}

void ezJoltCustomShapeInfo::GetTrianglesStart(GetTrianglesContext& ref_context, const AABox& box, Vec3Arg inPositionCOM, QuatArg inRotation, Vec3Arg inScale) const
{
  mInnerShape->GetTrianglesStart(ref_context, box, inPositionCOM, inRotation, inScale);
}

int ezJoltCustomShapeInfo::GetTrianglesNext(GetTrianglesContext& ref_context, int iInMaxTrianglesRequested, Float3* pTriangleVertices, const PhysicsMaterial** pMaterials /*= nullptr*/) const
{
  return mInnerShape->GetTrianglesNext(ref_context, iInMaxTrianglesRequested, pTriangleVertices, pMaterials);
}

float ezJoltCustomShapeInfo::GetVolume() const
{
  return mInnerShape->GetVolume();
}

Shape::Stats ezJoltCustomShapeInfo::GetStatsRecursive(VisitedShapes& ref_ioVisitedShapes) const
{
  return mInnerShape->GetStatsRecursive(ref_ioVisitedShapes);
}

void ezJoltCustomShapeInfo::sRegister()
{
  ShapeFunctions& f = ShapeFunctions::sGet(EShapeSubType::User1);
  f.mConstruct = []() -> Shape*
  { return new ezJoltCustomShapeInfo; };
  f.mColor = Color::sCyan;

  for (EShapeSubType s : sAllSubShapeTypes)
  {
    CollisionDispatch::sRegisterCollideShape(EShapeSubType::User1, s, sCollideUser1VsShape);
    CollisionDispatch::sRegisterCollideShape(s, EShapeSubType::User1, sCollideShapeVsUser1);
    CollisionDispatch::sRegisterCastShape(EShapeSubType::User1, s, sCastUser1VsShape);
    CollisionDispatch::sRegisterCastShape(s, EShapeSubType::User1, sCastShapeVsUser1);
  }
}

void ezJoltCustomShapeInfo::CollideSoftBodyVertices(JPH::Mat44Arg centerOfMassTransform, JPH::Vec3Arg scale, const JPH::CollideSoftBodyVertexIterator& vertices, JPH::uint numVertices, int iCollidingShapeIndex) const
{
  mInnerShape->CollideSoftBodyVertices(centerOfMassTransform, scale, vertices, numVertices, iCollidingShapeIndex);
}

void ezJoltCustomShapeInfo::CollectTransformedShapes(const JPH::AABox& box, JPH::Vec3Arg positionCOM, JPH::QuatArg rotation, JPH::Vec3Arg scale, const JPH::SubShapeIDCreator& subShapeIDCreator, JPH::TransformedShapeCollector& ref_ioCollector, const JPH::ShapeFilter& shapeFilter) const
{
  mInnerShape->CollectTransformedShapes(box, positionCOM, rotation, scale, subShapeIDCreator, ref_ioCollector, shapeFilter);
}

void ezJoltCustomShapeInfo::sCollideUser1VsShape(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter)
{
  JPH_ASSERT(inShape1->GetSubType() == EShapeSubType::User1);
  const ezJoltCustomShapeInfo* shape1 = static_cast<const ezJoltCustomShapeInfo*>(inShape1);

  CollisionDispatch::sCollideShapeVsShape(shape1->mInnerShape, inShape2, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, inCollideShapeSettings, ioCollector, inShapeFilter);
}

void ezJoltCustomShapeInfo::sCollideShapeVsUser1(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter)
{
  JPH_ASSERT(inShape2->GetSubType() == EShapeSubType::User1);
  const ezJoltCustomShapeInfo* shape2 = static_cast<const ezJoltCustomShapeInfo*>(inShape2);

  CollisionDispatch::sCollideShapeVsShape(inShape1, shape2->mInnerShape, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, inCollideShapeSettings, ioCollector, inShapeFilter);
}

void ezJoltCustomShapeInfo::sCastUser1VsShape(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector)
{
  // Fetch offset center of mass shape from cast shape
  JPH_ASSERT(inShapeCast.mShape->GetSubType() == EShapeSubType::User1);

  JPH::ShapeCast innerShapeCast(static_cast<const ezJoltCustomShapeInfo*>(inShapeCast.mShape)->GetInnerShape(), inShapeCast.mScale, inShapeCast.mCenterOfMassStart, inShapeCast.mDirection);

  CollisionDispatch::sCastShapeVsShapeLocalSpace(innerShapeCast, inShapeCastSettings, inShape, inScale, inShapeFilter, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, ioCollector);
}

void ezJoltCustomShapeInfo::sCastShapeVsUser1(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector)
{
  JPH_ASSERT(inShape->GetSubType() == EShapeSubType::User1);
  const ezJoltCustomShapeInfo* shape = static_cast<const ezJoltCustomShapeInfo*>(inShape);

  CollisionDispatch::sCastShapeVsShapeLocalSpace(inShapeCast, inShapeCastSettings, shape->mInnerShape, inScale, inShapeFilter, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, ioCollector);
}
