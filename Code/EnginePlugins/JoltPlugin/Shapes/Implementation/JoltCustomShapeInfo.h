#pragma once

#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>

class ezJoltCustomShapeInfo : public JPH::DecoratedShape
{
public:
  ezJoltCustomShapeInfo()
    : DecoratedShape(JPH::EShapeSubType::User1)
  {
  }

  ezJoltCustomShapeInfo(const Shape* inInnerShape)
    : DecoratedShape(JPH::EShapeSubType::User1, inInnerShape)
  {
  }

  float m_fDensity = 1.0f;
  ezHybridArray< JPH::RefConst<JPH::PhysicsMaterial>, 1> m_CustomMaterials;

  virtual const JPH::PhysicsMaterial* GetMaterial(const JPH::SubShapeID& inSubShapeID) const override;
  virtual JPH::uint64 GetSubShapeUserData(const JPH::SubShapeID& inSubShapeID) const override;
  virtual JPH::MassProperties GetMassProperties() const override;
  virtual JPH::Vec3	GetCenterOfMass() const override;

  // all these just pass through to the inner shape
  virtual JPH::AABox GetLocalBounds() const override;
  virtual float GetInnerRadius() const override;
  virtual JPH::Vec3 GetSurfaceNormal(const JPH::SubShapeID& inSubShapeID, JPH::Vec3Arg inLocalSurfacePosition) const override;
  virtual void GetSubmergedVolume(JPH::Mat44Arg inCenterOfMassTransform, JPH::Vec3Arg inScale, const JPH::Plane& inSurface, float& outTotalVolume, float& outSubmergedVolume, JPH::Vec3& outCenterOfBuoyancy) const override;
  virtual void Draw(JPH::DebugRenderer* inRenderer, JPH::Mat44Arg inCenterOfMassTransform, JPH::Vec3Arg inScale, JPH::ColorArg inColor, bool inUseMaterialColors, bool inDrawWireframe) const override;
  virtual bool CastRay(const JPH::RayCast& inRay, const JPH::SubShapeIDCreator& inSubShapeIDCreator, JPH::RayCastResult& ioHit) const override;
  virtual void CastRay(const JPH::RayCast& inRay, const JPH::RayCastSettings& inRayCastSettings, const JPH::SubShapeIDCreator& inSubShapeIDCreator, JPH::CastRayCollector& ioCollector) const override;
  virtual void CollidePoint(JPH::Vec3Arg inPoint, const JPH::SubShapeIDCreator& inSubShapeIDCreator, JPH::CollidePointCollector& ioCollector) const override;
  virtual void GetTrianglesStart(JPH::Shape::GetTrianglesContext& ioContext, const JPH::AABox& inBox, JPH::Vec3Arg inPositionCOM, JPH::QuatArg inRotation, JPH::Vec3Arg inScale) const override;
  virtual int GetTrianglesNext(JPH::Shape::GetTrianglesContext& ioContext, int inMaxTrianglesRequested, JPH::Float3* outTriangleVertices, const JPH::PhysicsMaterial** outMaterials = nullptr) const override;
  virtual Stats GetStats() const override { return Stats(sizeof(*this), 0); }
  virtual float GetVolume() const override;

  // Register shape functions with the registry
  static void sRegister();

private:
  // Helper functions called by CollisionDispatch
  static void sCollideUser1VsShape(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector);
  static void sCollideShapeVsUser1(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector);
  static void sCastUser1VsShape(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector);
  static void sCastShapeVsUser1(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector);
};
