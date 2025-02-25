#pragma once

#include <Jolt/Physics/Collision/Shape/DecoratedShape.h>

class ezJoltCustomShapeInfo : public JPH::DecoratedShape
{
public:
  ezJoltCustomShapeInfo()
    : DecoratedShape(JPH::EShapeSubType::User1)
  {
  }

  ezJoltCustomShapeInfo(const Shape* pInInnerShape)
    : DecoratedShape(JPH::EShapeSubType::User1, pInInnerShape)
  {
  }

  float m_fDensity = 1.0f;
  ezHybridArray<JPH::RefConst<JPH::PhysicsMaterial>, 1> m_CustomMaterials;

  virtual const JPH::PhysicsMaterial* GetMaterial(const JPH::SubShapeID& subShapeID) const override;
  virtual JPH::uint64 GetSubShapeUserData(const JPH::SubShapeID& subShapeID) const override;
  virtual JPH::MassProperties GetMassProperties() const override;
  virtual JPH::Vec3 GetCenterOfMass() const override;

  // all these just pass through to the inner shape
  virtual JPH::AABox GetLocalBounds() const override;
  virtual float GetInnerRadius() const override;
  virtual JPH::Vec3 GetSurfaceNormal(const JPH::SubShapeID& subShapeID, JPH::Vec3Arg inLocalSurfacePosition) const override;
  virtual void GetSubmergedVolume(JPH::Mat44Arg centerOfMassTransform, JPH::Vec3Arg inScale, const JPH::Plane& surface, float& out_fTotalVolume, float& out_fSubmergedVolume, JPH::Vec3& out_centerOfBuoyancy
#ifdef JPH_DEBUG_RENDERER // Not using JPH_IF_DEBUG_RENDERER for Doxygen
    ,
    JPH::RVec3Arg inBaseOffset
#endif

  ) const override;
  virtual void Draw(JPH::DebugRenderer* pInRenderer, JPH::Mat44Arg centerOfMassTransform, JPH::Vec3Arg inScale, JPH::ColorArg inColor, bool bInUseMaterialColors, bool bInDrawWireframe) const override;
  virtual bool CastRay(const JPH::RayCast& ray, const JPH::SubShapeIDCreator& subShapeIDCreator, JPH::RayCastResult& ref_hit) const override;
  virtual void CastRay(const JPH::RayCast& ray, const JPH::RayCastSettings& rayCastSettings, const JPH::SubShapeIDCreator& subShapeIDCreator, JPH::CastRayCollector& ref_collector, const JPH::ShapeFilter& shapeFilter) const override;
  virtual void CollidePoint(JPH::Vec3Arg inPoint, const JPH::SubShapeIDCreator& subShapeIDCreator, JPH::CollidePointCollector& ref_collector, const JPH::ShapeFilter& shapeFilter) const override;
  virtual void GetTrianglesStart(JPH::Shape::GetTrianglesContext& ref_context, const JPH::AABox& box, JPH::Vec3Arg inPositionCOM, JPH::QuatArg inRotation, JPH::Vec3Arg inScale) const override;
  virtual int GetTrianglesNext(JPH::Shape::GetTrianglesContext& ref_context, int iInMaxTrianglesRequested, JPH::Float3* pTriangleVertices, const JPH::PhysicsMaterial** pMaterials = nullptr) const override;
  virtual Stats GetStats() const override { return Stats(sizeof(*this), 0); }
  virtual float GetVolume() const override;

  virtual Stats GetStatsRecursive(VisitedShapes& ref_ioVisitedShapes) const override;

  // Register shape functions with the registry
  static void sRegister();

  void CollideSoftBodyVertices(JPH::Mat44Arg centerOfMassTransform, JPH::Vec3Arg scale, const JPH::CollideSoftBodyVertexIterator& vertices, JPH::uint numVertices, int iCollidingShapeIndex) const override;

  virtual void CollectTransformedShapes(const JPH::AABox& box, JPH::Vec3Arg positionCOM, JPH::QuatArg rotation, JPH::Vec3Arg scale, const JPH::SubShapeIDCreator& subShapeIDCreator, JPH::TransformedShapeCollector& ref_ioCollector, const JPH::ShapeFilter& shapeFilter) const override;

private:
  // Helper functions called by CollisionDispatch
  static void sCollideUser1VsShape(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter);
  static void sCollideShapeVsUser1(const JPH::Shape* inShape1, const JPH::Shape* inShape2, JPH::Vec3Arg inScale1, JPH::Vec3Arg inScale2, JPH::Mat44Arg inCenterOfMassTransform1, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, const JPH::CollideShapeSettings& inCollideShapeSettings, JPH::CollideShapeCollector& ioCollector, const JPH::ShapeFilter& inShapeFilter);
  static void sCastUser1VsShape(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector);
  static void sCastShapeVsUser1(const JPH::ShapeCast& inShapeCast, const JPH::ShapeCastSettings& inShapeCastSettings, const Shape* inShape, JPH::Vec3Arg inScale, const JPH::ShapeFilter& inShapeFilter, JPH::Mat44Arg inCenterOfMassTransform2, const JPH::SubShapeIDCreator& inSubShapeIDCreator1, const JPH::SubShapeIDCreator& inSubShapeIDCreator2, JPH::CastShapeCollector& ioCollector);
};
