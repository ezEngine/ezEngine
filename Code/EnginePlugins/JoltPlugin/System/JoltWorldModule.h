#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

namespace JPH
{
  class TempAllocator;
  class PhysicsSystem;
} // namespace JPH

class EZ_JOLTPLUGIN_DLL ezJoltWorldModule : public ezPhysicsWorldModuleInterface
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltWorldModule, ezPhysicsWorldModuleInterface);

public:
  ezJoltWorldModule(ezWorld* pWorld);
  ~ezJoltWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  JPH::PhysicsSystem* GetJoltSystem() { return m_pSystem.Borrow(); }
  const JPH::PhysicsSystem* GetJoltSystem() const { return m_pSystem.Borrow(); }

  ezUInt32 CreateObjectFilterID();
  void DeleteObjectFilterID(ezUInt32& uiObjectFilterID);

  ezUInt32 AllocateUserData(ezJoltUserData*& out_pUserData);
  void DeallocateUserData(ezUInt32& uiUserDataId);
  const ezJoltUserData& GetUserData(ezUInt32 uiUserDataId) const;

  void SetGravity(const ezVec3& objectGravity, const ezVec3& characterGravity);
  virtual ezVec3 GetGravity() const override { return ezVec3(0, 0, -10); }
  ezVec3 GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }

  //////////////////////////////////////////////////////////////////////////
  // ezPhysicsWorldModuleInterface

  virtual bool Raycast(ezPhysicsCastResult& out_Result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool RaycastAll(ezPhysicsCastResultArray& out_Results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const override;

  virtual bool SweepTestSphere(ezPhysicsCastResult& out_Result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestBox(ezPhysicsCastResult& out_Result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCapsule(ezPhysicsCastResult& out_Result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInSphere(ezPhysicsOverlapResultArray& out_Results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual void AddStaticCollisionBox(ezGameObject* pObject, ezVec3 boxSize) override;

  //ezMap<physx::PxConstraint*, ezComponentHandle> m_BreakableJoints;
  ezDeque<ezComponentHandle> m_RequireUpdate;

private:
  bool SweepTest(ezPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const;
  bool OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const ezPhysicsQueryParameters& params) const;

  void FreeUserDataAfterSimulationStep();

  void StartSimulation(const ezWorldModule::UpdateContext& context);
  void FetchResults(const ezWorldModule::UpdateContext& context);

  //void HandleSimulationEvents();

  //void UpdatePhysicsSlideReactions();
  //void UpdatePhysicsRollReactions();

  //void SpawnPhysicsImpactReactions();

  //void HandleBrokenConstraints();
  //void HandleTriggerEvents();

  void Simulate();
  void SimulateStep(ezTime deltaTime);

  void UpdateSettingsCfg();
  void ApplySettingsCfg();

  void UpdateConstraints();

  //ezJoltSimulationEventCallback* m_pSimulationEventCallback = nullptr;

  ezUInt32 m_uiNextObjectFilterID = 1;
  ezDynamicArray<ezUInt32> m_FreeObjectFilterIDs;

  ezDeque<ezJoltUserData> m_AllocatedUserData;
  ezDynamicArray<ezUInt32> m_FreeUserData;
  ezDynamicArray<ezUInt32> m_FreeUserDataAfterSimulationStep;

  ezTime m_AccumulatedTimeSinceUpdate;

  ezJoltSettings m_Settings;

  //ezSharedPtr<ezTask> m_pSimulateTask;
  //ezTaskGroupID m_SimulateTaskGroupId;
  //bool m_bSimulationStepExecuted = false;

  ezUniquePtr<JPH::PhysicsSystem> m_pSystem;
  ezUniquePtr<JPH::TempAllocator> m_pTempAllocator;

  ezJoltObjectToBroadphaseLayer m_ObjectToBroadphase;
};
