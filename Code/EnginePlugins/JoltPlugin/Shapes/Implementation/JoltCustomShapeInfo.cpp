#include <JoltPlugin/JoltPluginPCH.h>

#include <Jolt/AABBTree/TriangleCodec/TriangleCodecIndexed8BitPackSOA4Flags.h>
#include <Jolt/Physics/Collision/CollisionDispatch.h>
#include <JoltPlugin/Shapes/Implementation/JoltCustomShapeInfo.h>

using namespace JPH;

const JPH::PhysicsMaterial* ezJoltCustomShapeInfo::GetMaterial(const SubShapeID& inSubShapeID) const
{
  if (!m_CustomMaterials.IsEmpty())
  {
    if (mInnerShape->GetSubType() == EShapeSubType::Mesh)
    {
      const JPH::MeshShape* pMeshShape = static_cast<const JPH::MeshShape*>(mInnerShape.GetPtr());

      return m_CustomMaterials[pMeshShape->GetMaterialIndex(inSubShapeID)];
    }

    return m_CustomMaterials[0];
  }

  return mInnerShape->GetMaterial(inSubShapeID);
}

JPH::uint64 ezJoltCustomShapeInfo::GetSubShapeUserData(const SubShapeID& inSubShapeID) const
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

JPH::Vec3 ezJoltCustomShapeInfo::GetSurfaceNormal(const SubShapeID& inSubShapeID, Vec3Arg inLocalSurfacePosition) const
{
  return mInnerShape->GetSurfaceNormal(inSubShapeID, inLocalSurfacePosition);
}

void ezJoltCustomShapeInfo::GetSubmergedVolume(Mat44Arg inCenterOfMassTransform, Vec3Arg inScale, const Plane& inSurface, float& outTotalVolume, float& outSubmergedVolume, Vec3& outCenterOfBuoyancy) const
{
  mInnerShape->GetSubmergedVolume(inCenterOfMassTransform, inScale, inSurface, outTotalVolume, outSubmergedVolume, outCenterOfBuoyancy);
}

void ezJoltCustomShapeInfo::Draw(DebugRenderer* inRenderer, Mat44Arg inCenterOfMassTransform, Vec3Arg inScale, ColorArg inColor, bool inUseMaterialColors, bool inDrawWireframe) const
{
  mInnerShape->Draw(inRenderer, inCenterOfMassTransform, inScale, inColor, inUseMaterialColors, inDrawWireframe);
}

bool ezJoltCustomShapeInfo::CastRay(const RayCast& inRay, const SubShapeIDCreator& inSubShapeIDCreator, RayCastResult& ioHit) const
{
  return mInnerShape->CastRay(inRay, inSubShapeIDCreator, ioHit);
}

void ezJoltCustomShapeInfo::CastRay(const RayCast& inRay, const RayCastSettings& inRayCastSettings, const SubShapeIDCreator& inSubShapeIDCreator, CastRayCollector& ioCollector) const
{
  return mInnerShape->CastRay(inRay, inRayCastSettings, inSubShapeIDCreator, ioCollector);
}

void ezJoltCustomShapeInfo::CollidePoint(Vec3Arg inPoint, const SubShapeIDCreator& inSubShapeIDCreator, CollidePointCollector& ioCollector) const
{
  mInnerShape->CollidePoint(inPoint, inSubShapeIDCreator, ioCollector);
}

void ezJoltCustomShapeInfo::GetTrianglesStart(GetTrianglesContext& ioContext, const AABox& inBox, Vec3Arg inPositionCOM, QuatArg inRotation, Vec3Arg inScale) const
{
  mInnerShape->GetTrianglesStart(ioContext, inBox, inPositionCOM, inRotation, inScale);
}

int ezJoltCustomShapeInfo::GetTrianglesNext(GetTrianglesContext& ioContext, int inMaxTrianglesRequested, Float3* outTriangleVertices, const PhysicsMaterial** outMaterials /*= nullptr*/) const
{
  return mInnerShape->GetTrianglesNext(ioContext, inMaxTrianglesRequested, outTriangleVertices, outMaterials);
}

float ezJoltCustomShapeInfo::GetVolume() const
{
  return mInnerShape->GetVolume();
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

void ezJoltCustomShapeInfo::sCollideUser1VsShape(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector)
{
  JPH_ASSERT(inShape1->GetSubType() == EShapeSubType::User1);
  const ezJoltCustomShapeInfo* shape1 = static_cast<const ezJoltCustomShapeInfo*>(inShape1);

  CollisionDispatch::sCollideShapeVsShape(shape1->mInnerShape, inShape2, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, inCollideShapeSettings, ioCollector);
}

void ezJoltCustomShapeInfo::sCollideShapeVsUser1(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector)
{
  JPH_ASSERT(inShape2->GetSubType() == EShapeSubType::User1);
  const ezJoltCustomShapeInfo* shape2 = static_cast<const ezJoltCustomShapeInfo*>(inShape2);

  CollisionDispatch::sCollideShapeVsShape(inShape1, shape2->mInnerShape, inScale1, inScale2, inCenterOfMassTransform1, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, inCollideShapeSettings, ioCollector);
}

void ezJoltCustomShapeInfo::sCastUser1VsShape(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector)
{
  // Fetch offset center of mass shape from cast shape
  JPH_ASSERT(inShapeCast.mShape->GetSubType() == EShapeSubType::User1);
  const ezJoltCustomShapeInfo* shape1 = static_cast<const ezJoltCustomShapeInfo*>(inShapeCast.mShape);

  CollisionDispatch::sCastShapeVsShapeLocalSpace(inShapeCast, inShapeCastSettings, inShape, inScale, inShapeFilter, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, ioCollector);
}

void ezJoltCustomShapeInfo::sCastShapeVsUser1(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector)
{
  JPH_ASSERT(inShape->GetSubType() == EShapeSubType::User1);
  const ezJoltCustomShapeInfo* shape = static_cast<const ezJoltCustomShapeInfo*>(inShape);

  CollisionDispatch::sCastShapeVsShapeLocalSpace(inShapeCast, inShapeCastSettings, shape->mInnerShape, inScale, inShapeFilter, inCenterOfMassTransform2, inSubShapeIDCreator1, inSubShapeIDCreator2, ioCollector);
}
