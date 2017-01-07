#pragma once

#include <PhysXPlugin/Basics.h>
#include <Core/World/WorldModule.h>
#include <GameUtils/Surfaces/SurfaceResource.h>
#include <GameUtils/CollisionFilter/CollisionFilter.h>
#include <GameUtils/Interfaces/PhysicsWorldModule.h>

namespace physx
{
  class PxScene;
  class PxControllerManager;
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

  physx::PxScene* GetPxScene() const { return m_pPxScene; }
  physx::PxControllerManager* GetCharacterManager() const { return m_pCharacterManager; }

  virtual ezVec3 GetGravity() const override { return m_vObjectGravity; }
  ezVec3 GetCharacterGravity() const { return m_vCharacterGravity; }

  void SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity);

  virtual bool CastRay(const ezVec3& vStart, const ezVec3& vDir, float fMaxLen, ezUInt8 uiCollisionLayer, ezVec3& out_vHitPos, ezVec3& out_vHitNormal, ezGameObjectHandle& out_hHitGameObject, ezSurfaceResourceHandle& out_hSurface) override;

  virtual bool SweepTestCapsule(const ezTransform& start, const ezVec3& vDir, float fCapsuleRadius, float fCapsuleHeight, float fDistance, ezUInt8 uiCollisionLayer, float& out_fDistance, ezVec3& out_Position, ezVec3& out_Normal) override;

private:
  void StartSimulation(const ezWorldModule::UpdateContext& context);
  void FetchResults(const ezWorldModule::UpdateContext& context);

  void Simulate();

  physx::PxScene* m_pPxScene;
  physx::PxControllerManager* m_pCharacterManager;

  ezTime m_AccumulatedTimeSinceUpdate;
  ezVec3 m_vObjectGravity;
  ezVec3 m_vCharacterGravity;

  ezDelegateTask<void> m_SimulateTask;
  ezTaskGroupID m_SimulateTaskGroupId;

  EZ_MAKE_SUBSYSTEM_STARTUP_FRIEND(PhysX, PhysXPlugin);
};

#define EZ_PX_READ_LOCK(scene) \
  physx::PxSceneReadLock EZ_CONCAT(pxl_, EZ_SOURCE_LINE)(scene, EZ_SOURCE_FILE, EZ_SOURCE_LINE)

#define EZ_PX_WRITE_LOCK(scene) \
  physx::PxSceneWriteLock EZ_CONCAT(pxl_, EZ_SOURCE_LINE)(scene, EZ_SOURCE_FILE, EZ_SOURCE_LINE)
