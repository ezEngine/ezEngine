#pragma once

#include <Core/Interfaces/NavmeshGeoWorldModule.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>

class ezJoltCharacterControllerComponent;
class ezJoltContactListener;
class ezJoltRagdollComponent;
class ezJoltRopeComponent;
class ezView;

namespace JPH
{
  class Body;
  class TempAllocator;
  class PhysicsSystem;
  class GroupFilter;
} // namespace JPH


struct ezJoltImpulse
{
  ezUInt32 m_uiBodyID;
  ezVec3 m_vImpulse;
  ezVec3 m_vGlobalPosition;
};

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


  /// \brief Queues an impulse to be applied on the given body as soon as that body is added to the Jolt scene.
  void AddImpulse(ezUInt32 uiBodyID, const ezVec3& vImpulse, const ezVec3& vGlobalPosition);

  //////////////////////////////////////////////////////////////////////////
  // ezPhysicsWorldModuleInterface
  //

  virtual ezUInt32 GetCollisionLayerByName(ezStringView sName) const override;

  virtual bool Raycast(ezPhysicsCastResult& out_result, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool RaycastAll(ezPhysicsCastResultArray& out_results, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params) const override;

  virtual bool SweepTestSphere(ezPhysicsCastResult& out_result, float fSphereRadius, const ezVec3& vStart, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestBox(ezPhysicsCastResult& out_result, const ezVec3& vBoxExtents, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCapsule(ezPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCylinder(ezPhysicsCastResult& out_result, float fCylinderRadius, float fCylinderHeight, const ezTransform& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection = ezPhysicsHitCollection::Closest) const override;

  virtual bool OverlapTestSphere(float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestBox(const ezVec3& vBoxExtents, const ezVec3& vPosition, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCylinder(float fCylinderRadius, float fCylinderHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInSphere(ezPhysicsOverlapResultArray& out_results, float fSphereRadius, const ezVec3& vPosition, const ezPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInBox(ezPhysicsOverlapResultArray& out_results, const ezVec3& vBoxExtents, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInCapsule(ezPhysicsOverlapResultArray& out_results, float fCapsuleRadius, float fCapsuleHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInCylinder(ezPhysicsOverlapResultArray& out_results, float fCylinderRadius, float fCylinderHeight, const ezTransform& transform, const ezPhysicsQueryParameters& params) const override;

  virtual void AddStaticCollisionBox(ezGameObject* pObject, ezVec3 vBoxSize) override;

  virtual void AddFixedJointComponent(ezGameObject* pOwner, const ezPhysicsWorldModuleInterface::FixedJointConfig& cfg) override;

  virtual ezBoundingBoxSphere GetWorldSpaceBounds(ezGameObject* pOwner, ezUInt32 uiCollisionLayer, ezBitflags<ezPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const override;

  ezDeque<ezComponentHandle> m_RequireUpdate;

  const ezSet<ezJoltDynamicActorComponent*>& GetActiveActors() const { return m_ActiveActors; }
  const ezMap<ezJoltRagdollComponent*, ezInt32>& GetActiveRagdolls() const { return m_ActiveRagdolls; }
  const ezMap<ezJoltRopeComponent*, ezInt32>& GetActiveRopes() const { return m_ActiveRopes; }
  ezArrayPtr<ezJoltRagdollComponent*> GetRagdollsPutToSleep() { return m_RagdollsPutToSleep.GetArrayPtr(); }

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

  void QueryGeometryInBox(const ezPhysicsQueryParameters& params, ezBoundingBox box, ezDynamicArray<ezNavmeshTriangle>& out_triangles) const;

  /// \brief Returns the counter of the last Jolt update.
  /// Can be used to detect when no physics update was done (at high frame rates) to skip duplicate physics modifications.
  ezUInt64 GetJoltUpdateCounter() const { return m_uiJoltUpdateCounter; }

private:
  bool SweepTest(ezPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const ezVec3& vDir, float fDistance, const ezPhysicsQueryParameters& params, ezPhysicsHitCollection collection) const;
  bool OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const ezPhysicsQueryParameters& params) const;
  void QueryShapes(ezPhysicsOverlapResultArray& out_results, const JPH::Shape& shape, const JPH::Mat44& transform, const ezPhysicsQueryParameters& params) const;

  void FreeUserDataAfterSimulationStep();

  void StartSimulation(const ezWorldModule::UpdateContext& context);
  void FetchResults(const ezWorldModule::UpdateContext& context);

  void Simulate();

  void UpdateSettingsCfg();
  void ApplySettingsCfg();

  void ApplyImpulses();

  void UpdateConstraints();

  ezTime CalculateUpdateSteps();

  void DebugDrawGeometry();
  void DebugDrawGeometry(const ezVec3& vCenter, float fRadius, ezPhysicsShapeType::Enum shapeType, const ezTag& tag);

  struct DebugGeo
  {
    ezGameObjectHandle m_hObject;
    ezUInt32 m_uiLastSeenCounter = 0;
    bool m_bMutableGeometry = false;
  };

  struct DebugGeoShape
  {
    ezDynamicMeshBufferResourceHandle m_hMesh;
    ezBoundingBox m_Bounds;
    ezUInt32 m_uiLastSeenCounter = 0;
  };

  struct DebugBodyShapeKey
  {
    ezUInt32 m_uiBodyID;
    const void* m_pShapePtr;

    bool operator<(const DebugBodyShapeKey& rhs) const
    {
      if (m_uiBodyID == rhs.m_uiBodyID)
        return m_pShapePtr < rhs.m_pShapePtr;

      return m_uiBodyID < rhs.m_uiBodyID;
    }

    bool operator==(const DebugBodyShapeKey& rhs) const
    {
      return (m_uiBodyID == rhs.m_uiBodyID) && (m_pShapePtr == rhs.m_pShapePtr);
    }
  };

  ezUInt64 m_uiJoltUpdateCounter = 0;

  ezUInt32 m_uiDebugGeoLastSeenCounter = 0;
  ezMap<DebugBodyShapeKey, DebugGeo> m_DebugDrawComponents;
  ezMap<const void*, DebugGeoShape> m_DebugDrawShapeGeo;

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
  ezSet<ezJoltDynamicActorComponent*> m_ActiveActors;
  ezMap<ezJoltRagdollComponent*, ezInt32> m_ActiveRagdolls;
  ezMap<ezJoltRopeComponent*, ezInt32> m_ActiveRopes;
  ezDynamicArray<ezJoltRagdollComponent*> m_RagdollsPutToSleep;

  JPH::GroupFilter* m_pGroupFilter = nullptr;
  JPH::GroupFilter* m_pGroupFilterIgnoreSame = nullptr;

  ezUInt32 m_uiBodiesAddedSinceOptimize = 100;
  ezDeque<ezUInt32> m_BodiesToAdd;
  ezDeque<ezUInt32> m_BodiesToAddAndActivate;

  ezHybridArray<ezTime, 4> m_UpdateSteps;
  ezHybridArray<ezJoltCharacterControllerComponent*, 4> m_ActiveCharacters;

  ezMutex m_ImpulsesMutex;
  ezDeque<ezJoltImpulse> m_Impulses;
};

/// \brief Implementation of the ezNavmeshGeoWorldModuleInterface that uses Jolt physics to retrieve the geometry
/// from which to generate a navmesh.
class EZ_JOLTPLUGIN_DLL ezJoltNavmeshGeoWorldModule : public ezNavmeshGeoWorldModuleInterface
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezJoltNavmeshGeoWorldModule, ezNavmeshGeoWorldModuleInterface);

public:
  ezJoltNavmeshGeoWorldModule(ezWorld* pWorld);

  virtual void RetrieveGeometryInArea(ezUInt32 uiCollisionLayer, const ezBoundingBox& box, ezDynamicArray<ezNavmeshTriangle>& out_triangles) const override;

private:
  ezJoltWorldModule* m_pJoltModule = nullptr;
};
