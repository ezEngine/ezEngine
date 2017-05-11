#pragma once

#include <GameEngine/Basics.h>
#include <Core/World/WorldModule.h>
#include <Core/ResourceManager/ResourceHandle.h>

class ezGameObjectHandle;

typedef ezTypedResourceHandle<class ezSurfaceResource> ezSurfaceResourceHandle;

struct ezPhysicsHitResult
{
  ezVec3 m_vPosition;
  ezVec3 m_vNormal;
  float m_fDistance;
  void* m_pShape = nullptr; // the object inside the physics engine that was hit

  ezGameObjectHandle m_hGameObject;
  ezSurfaceResourceHandle m_hSurface;
};

class EZ_GAMEENGINE_DLL ezPhysicsWorldModuleInterface : public ezWorldModule
{
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysicsWorldModuleInterface, ezWorldModule);

protected:
  ezPhysicsWorldModuleInterface(ezWorld* pWorld) : ezWorldModule(pWorld) { }

public:

  virtual bool CastRay(const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer,
    ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) = 0;

  virtual bool SweepTestSphere(float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) = 0;

  virtual bool SweepTestBox(ezVec3 vBoxExtends, const ezTransform& start, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) = 0;

  virtual bool SweepTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& start, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) = 0;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) = 0;
  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& vPosition, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) = 0;

  virtual void ApplyImpulseAtPos(void* pTargetShape, const ezVec3& vPosition, const ezVec3& vImpulse) = 0;

  virtual ezVec3 GetGravity() const = 0;

private:

};
