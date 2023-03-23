#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

class ezJoltCharacterControllerComponent;
class ezJoltContactListener;

namespace JPH
{
  class Body;
  class TempAllocator;
  class PhysicsSystem;
  class GroupFilter;
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

  JPH::PhysicsSystem* GetJoltSystem() { return m_pSystem.get(); }
  const JPH::PhysicsSystem* GetJoltSystem() const { return m_pSystem.get(); }

  ezUInt32 CreateObjectFilterID();
  void DeleteObjectFilterID(ezUInt32& ref_uiObjectFilterID);

  ezUInt32 AllocateUserData(ezJoltUserData*& out_pUserData);
  void DeallocateUserData(ezUInt32& ref_uiUserDataId);
  const ezJoltUserData& GetUserData(ezUInt32 uiUserDataId) const;

  void SetGravity(const ezVec3& vObjectGravity, const ezVec3& vCharacterGravity);
  virtual ezVec3 GetGravity() const override { return ezVec3(0, 0, -10); }
  ezVec3 GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }

  //////////////////////////////////////////////////////////////////////////
  // ezPhysicsWorldModuleInterface

  virtual bool Raycast(ezPhysicsCastResult& out_result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool RaycastAll(ezPhysicsCastResultArray& out_results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const override;

  virtual bool SweepTestSphere(ezPhysicsCastResult& out_result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestBox(ezPhysicsCastResult& out_result, ezVec3 vBoxExtends, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCapsule(ezPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInSphere(ezPhysicsOverlapResultArray& out_results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual void AddStaticCollisionBox(ezGameObject* pObject, ezVec3 vBoxSize) override;

  virtual void AddFixedJointComponent(ezGameObject* pOwner, const ezPhysicsWorldModuleInterface::FixedJointConfig& cfg) override;

  ezDeque<ezComponentHandle> m_RequireUpdate;

  const ezMap<ezJoltActorComponent*, ezUInt32>& GetActiveActors() const { return m_ActiveActors; }

  void QueueBodyToAdd(JPH::Body* pBody, bool bAwake);

  JPH::GroupFilter* GetGroupFilter() const { return m_pGroupFilter; }
  JPH::GroupFilter* GetGroupFilterIgnoreSame() const { return m_pGroupFilterIgnoreSame; }

  void EnableJoinedBodiesCollisions(ezUInt32 uiObjectFilterID1, ezUInt32 uiObjectFilterID2, bool bEnable);

  JPH::TempAllocator* GetTempAllocator() const { return m_pTempAllocator.get(); }

  void ActivateCharacterController(ezJoltCharacterControllerComponent* pCharacter, bool bActivate);

  ezJoltContactListener* GetContactListener()
  {
    return reinterpret_cast<ezJoltContactListener*>(m_pContactListener);
  }

  void CheckBreakableConstraints();

  ezSet<ezComponentHandle> m_BreakableConstraints;

private:
  bool SweepTest(ezPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const;
  bool OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const ezPhysicsQueryParameters& params) const;

  void FreeUserDataAfterSimulationStep();

  void StartSimulation(const ezWorldModule::UpdateContext& context);
  void FetchResults(const ezWorldModule::UpdateContext& context);

  void Simulate();

  void UpdateSettingsCfg();
  void ApplySettingsCfg();

  void UpdateConstraints();

  ezTime CalculateUpdateSteps();

  ezUInt32 m_uiNextObjectFilterID = 1;
  ezDynamicArray<ezUInt32> m_FreeObjectFilterIDs;

  ezDeque<ezJoltUserData> m_AllocatedUserData;
  ezDynamicArray<ezUInt32> m_FreeUserData;
  ezDynamicArray<ezUInt32> m_FreeUserDataAfterSimulationStep;

  ezTime m_AccumulatedTimeSinceUpdate;

  ezJoltSettings m_Settings;

  ezSharedPtr<ezTask> m_pSimulateTask;
  ezTaskGroupID m_SimulateTaskGroupId;
  ezTime m_SimulatedTimeStep;

  std::unique_ptr<JPH::PhysicsSystem> m_pSystem;
  std::unique_ptr<JPH::TempAllocator> m_pTempAllocator;

  ezJoltObjectToBroadphaseLayer m_ObjectToBroadphase;
  ezJoltObjectVsBroadPhaseLayerFilter m_ObjectVsBroadphaseFilter;
  ezJoltObjectLayerPairFilter m_ObjectLayerPairFilter;

  void* m_pContactListener = nullptr;
  void* m_pActivationListener = nullptr;
  ezMap<ezJoltActorComponent*, ezUInt32> m_ActiveActors;

  JPH::GroupFilter* m_pGroupFilter = nullptr;
  JPH::GroupFilter* m_pGroupFilterIgnoreSame = nullptr;

  ezUInt32 m_uiBodiesAddedSinceOptimize = 100;
  ezDeque<ezUInt32> m_BodiesToAdd;
  ezDeque<ezUInt32> m_BodiesToAddAndActivate;

  ezHybridArray<ezTime, 4> m_UpdateSteps;
  ezHybridArray<ezJoltCharacterControllerComponent*, 4> m_ActiveCharacters;
};
