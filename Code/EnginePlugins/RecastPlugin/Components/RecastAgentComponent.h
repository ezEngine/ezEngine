#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/AI/AgentSteeringComponent.h>
#include <Recast/DetourNavMeshQuery.h>
#include <Recast/DetourPathCorridor.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/RecastPluginDLL.h>

class ezRecastWorldModule;
class ezPhysicsWorldModuleInterface;
struct ezResourceEvent;

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcAgentComponentManager : public ezComponentManager<class ezRcAgentComponent, ezBlockStorageType::FreeList>
{
  typedef ezComponentManager<class ezRcAgentComponent, ezBlockStorageType::FreeList> SUPER;

public:
  ezRcAgentComponentManager(ezWorld* pWorld);
  ~ezRcAgentComponentManager();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  ezRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

private:
  void ResourceEventHandler(const ezResourceEvent& e);
  void Update(const ezWorldModule::UpdateContext& context);

  ezPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
  ezRecastWorldModule* m_pWorldModule = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcAgentComponent : public ezAgentSteeringComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRcAgentComponent, ezAgentSteeringComponent, ezRcAgentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAgentSteeringComponent

public:
  ezRcAgentComponent();
  ~ezRcAgentComponent();

  virtual void SetTargetPosition(const ezVec3& vPosition) override;
  virtual ezVec3 GetTargetPosition() const override;
  virtual void ClearTargetPosition() override;
  virtual ezAgentPathFindingState::Enum GetPathToTargetState() const override;

  //////////////////////////////////////////////////////////////////////////
  // Helper Functions

public:
  ezResult FindNavMeshPolyAt(const ezVec3& vPosition, dtPolyRef& out_PolyRef, ezVec3* out_vAdjustedPosition = nullptr,
    float fPlaneEpsilon = 0.01f, float fHeightEpsilon = 1.0f) const;
  bool HasReachedPosition(const ezVec3& pos, float fMaxDistance) const;
  bool HasReachedGoal(float fMaxDistance) const;
  bool IsPositionVisible(const ezVec3& pos) const;

  //////////////////////////////////////////////////////////////////////////
  // Debug Visualization Functions

private:
  void VisualizePathCorridorPosition();
  void VisualizePathCorridor();
  void VisualizeCurrentPath();
  void VisualizeTargetPosition();

  //////////////////////////////////////////////////////////////////////////
  // Path Finding and Steering

private:
  ezResult ComputePathToTarget();
  ezResult ComputePathCorridor(dtPolyRef startPoly, dtPolyRef endPoly, bool& bFoundPartialPath);
  void ComputeSteeringDirection(float fMaxDistance);
  void ApplySteering(const ezVec3& vDirection, float fSpeed);
  void SyncSteeringWithReality();
  void PlanNextSteps();

  ezVec3 m_vTargetPosition;
  ezEnum<ezAgentPathFindingState> m_PathToTargetState;
  ezVec3 m_vCurrentPositionOnNavmesh;      /// \todo ??? keep update ?
  ezUniquePtr<dtNavMeshQuery> m_pQuery;    // careful, dtNavMeshQuery is not moveble
  ezUniquePtr<dtPathCorridor> m_pCorridor; // careful, dtPathCorridor is not moveble
  dtQueryFilter m_QueryFilter;             /// \todo hard-coded filter
  ezDynamicArray<dtPolyRef> m_PathCorridor;
  // path following
  ezInt32 m_iFirstNextStep = 0;
  ezInt32 m_iNumNextSteps = 0;
  ezVec3 m_vNextSteps[16];
  ezVec3 m_vCurrentSteeringDirection;


  //////////////////////////////////////////////////////////////////////////
  // Properties
public:
  float m_fWalkSpeed = 4.0f; // [ property ]
  /// \todo Expose and use
  // float m_fRadius = 0.2f;
  // float m_fHeight = 1.0f;


  //////////////////////////////////////////////////////////////////////////
  // Other
private:
  ezResult InitializeRecast();
  void UninitializeRecast();
  virtual void OnSimulationStarted() override;
  void Update();

  bool m_bRecastInitialized = false;
  ezComponentHandle m_hCharacterController;
};
