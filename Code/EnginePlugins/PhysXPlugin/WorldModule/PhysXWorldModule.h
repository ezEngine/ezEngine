#pragma once

#include <Core/World/WorldModule.h>
#include <Foundation/Threading/DelegateTask.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <PhysXPlugin/PhysXInterface.h>

class ezPxSimulationEventCallback;

class EZ_PHYSXPLUGIN_DLL ezPhysXWorldModule : public ezPhysicsWorldModuleInterface
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezPhysXWorldModule, ezPhysicsWorldModuleInterface);

public:
  ezPhysXWorldModule(ezWorld* pWorld);
  ~ezPhysXWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  physx::PxScene* GetPxScene() { return m_pPxScene; }
  const physx::PxScene* GetPxScene() const { return m_pPxScene; }
  physx::PxControllerManager* GetCharacterManager() { return m_pCharacterManager; }
  const physx::PxControllerManager* GetCharacterManager() const { return m_pCharacterManager; }

  ezUInt32 CreateShapeId();
  void DeleteShapeId(ezUInt32 uiShapeId);

  void SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity);
  virtual ezVec3 GetGravity() const override { return m_Settings.m_vObjectGravity; }
  ezVec3 GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }
  float GetMaxDepenetrationVelocity() const { return m_Settings.m_fMaxDepenetrationVelocity; }

  // ezPhysicsWorldModuleInterface implementation
  virtual bool CastRay(const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer,
    ezPhysicsHitResult& out_HitResult, ezBitflags<ezPhysicsShapeType> shapeTypes = ezPhysicsShapeType::Static | ezPhysicsShapeType::Dynamic,
    ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const override;

  virtual bool SweepTestSphere(float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer,
    ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const override;

  virtual bool SweepTestBox(ezVec3 vBoxExtends, const ezTransform& start, const ezVec3& vDir, float fDistance, ezUInt8 uiCollisionLayer,
    ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const override;

  virtual bool SweepTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& start, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const override;

  virtual bool OverlapTestSphere(
    float fSphereRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const override;
  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& vPosition, ezUInt8 uiCollisionLayer,
    ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const override;

  virtual void QueryDynamicShapesInSphere(float fSphereRadius, const ezVec3& vPosition, ezUInt8 uiCollisionLayer,
    ezPhysicsOverlapResult& out_Results, ezUInt32 uiIgnoreShapeId = ezInvalidIndex) const override;

  virtual void AddStaticCollisionBox(ezGameObject* pObject, ezVec3 boxSize) override;

  virtual void* CreateRagdoll(
    const ezSkeletonResourceDescriptor& skeleton, const ezTransform& rootTransform, const ezAnimationPose& initPose) override;

private:
  bool SweepTest(const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezVec3& vDir, float fDistance,
    ezUInt8 uiCollisionLayer, ezPhysicsHitResult& out_HitResult, ezUInt32 uiIgnoreShapeId) const;
  bool OverlapTest(
    const physx::PxGeometry& geometry, const physx::PxTransform& transform, ezUInt8 uiCollisionLayer, ezUInt32 uiIgnoreShapeId) const;

  void StartSimulation(const ezWorldModule::UpdateContext& context);
  void FetchResults(const ezWorldModule::UpdateContext& context);

  void Simulate();
  void SimulateStep(ezTime deltaTime);

  physx::PxScene* m_pPxScene;
  physx::PxControllerManager* m_pCharacterManager;
  ezPxSimulationEventCallback* m_pSimulationEventCallback;

  ezUInt32 m_uiNextShapeId;
  ezDynamicArray<ezUInt32> m_FreeShapeIds;

  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_ScratchMemory;

  ezTime m_AccumulatedTimeSinceUpdate;

  ezPxSettings m_Settings;

  ezDelegateTask<void> m_SimulateTask;
  ezTaskGroupID m_SimulateTaskGroupId;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXPlugin);
};

#define EZ_PX_READ_LOCK(scene) physx::PxSceneReadLock EZ_CONCAT(pxl_, EZ_SOURCE_LINE)(scene, EZ_SOURCE_FILE, EZ_SOURCE_LINE)

#define EZ_PX_WRITE_LOCK(scene) physx::PxSceneWriteLock EZ_CONCAT(pxl_, EZ_SOURCE_LINE)(scene, EZ_SOURCE_FILE, EZ_SOURCE_LINE)
