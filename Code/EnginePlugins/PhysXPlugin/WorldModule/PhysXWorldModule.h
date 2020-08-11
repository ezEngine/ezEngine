#pragma once

#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Threading/DelegateTask.h>
#include <GameEngine/Interfaces/PhysicsWorldModule.h>
#include <PhysXPlugin/PhysXInterface.h>
#include <PhysXPlugin/Utilities/PxUserData.h>

class ezPxSimulationEventCallback;

namespace physx
{
  class PxConstraint;
}

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
  void DeleteShapeId(ezUInt32& uiShapeId);

  ezUInt32 AllocateUserData(ezPxUserData*& out_pUserData);
  void DeallocateUserData(ezUInt32& uiUserDataId);
  ezPxUserData& GetUserData(ezUInt32 uiUserDataId);

  void SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity);
  virtual ezVec3 GetGravity() const override { return m_Settings.m_vObjectGravity; }
  ezVec3 GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }
  float GetMaxDepenetrationVelocity() const { return m_Settings.m_fMaxDepenetrationVelocity; }

  //////////////////////////////////////////////////////////////////////////
  // ezPhysicsWorldModuleInterface

  virtual bool Raycast(ezPhysicsCastResult& out_Result, const ezVec3& vStart, const ezVec3& vDir, float fDistance,
    const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool RaycastAll(ezPhysicsCastResultArray& out_Results, const ezVec3& vStart, const ezVec3& vDir, float fDistance,
    const ezPhysicsQueryParameters& params) const override;

  virtual bool SweepTestSphere(ezPhysicsCastResult& out_Result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance,
    const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestBox(ezPhysicsCastResult& out_Result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance,
    const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCapsule(ezPhysicsCastResult& out_Result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform,
    const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params,
    ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCapsule(
    float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInSphere(
    ezPhysicsOverlapResultArray& out_Results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual void AddStaticCollisionBox(ezGameObject* pObject, ezVec3 boxSize) override;

  virtual void* CreateRagdoll(const ezSkeletonResourceDescriptor& skeleton, const ezTransform& transform, const ezAnimationPose& initPose) override;

  ezMap<physx::PxConstraint*, ezComponentHandle> m_BreakableJoints;
  ezDeque<ezComponentHandle> m_RequireUpdate;

private:
  bool SweepTest(ezPhysicsCastResult& out_Result, const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezVec3& vDir,
    float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const;
  bool OverlapTest(const physx::PxGeometry& geometry, const physx::PxTransform& transform, const ezPhysicsQueryParameters& params) const;

  void FreeUserDataAfterSimulationStep();

  void StartSimulation(const ezWorldModule::UpdateContext& context);
  void FetchResults(const ezWorldModule::UpdateContext& context);

  void HandleBrokenConstraints();
  void HandleTriggerEvents();

  void Simulate();
  void SimulateStep(ezTime deltaTime);

  void UpdateJoints();

  physx::PxScene* m_pPxScene;
  physx::PxControllerManager* m_pCharacterManager;
  ezPxSimulationEventCallback* m_pSimulationEventCallback;

  ezUInt32 m_uiNextShapeId;
  ezDynamicArray<ezUInt32> m_FreeShapeIds;

  ezDeque<ezPxUserData> m_AllocatedUserData;
  ezDynamicArray<ezUInt32> m_FreeUserData;
  ezDynamicArray<ezUInt32> m_FreeUserDataAfterSimulationStep;

  ezDynamicArray<ezUInt8, ezAlignedAllocatorWrapper> m_ScratchMemory;

  ezTime m_AccumulatedTimeSinceUpdate;


  ezPxSettings m_Settings;

  ezSharedPtr<ezTask> m_pSimulateTask;
  ezTaskGroupID m_SimulateTaskGroupId;
  bool m_bSimulationStepExecuted = false;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXPlugin);
};

#define EZ_PX_READ_LOCK(scene) physx::PxSceneReadLock EZ_CONCAT(pxl_, EZ_SOURCE_LINE)(scene, EZ_SOURCE_FILE, EZ_SOURCE_LINE)

#define EZ_PX_WRITE_LOCK(scene) physx::PxSceneWriteLock EZ_CONCAT(pxl_, EZ_SOURCE_LINE)(scene, EZ_SOURCE_FILE, EZ_SOURCE_LINE)
