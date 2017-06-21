#pragma once

#include <RecastPlugin/Basics.h>
#include <Core/World/World.h>
#include <Core/World/Component.h>
#include <GameEngine/AI/NavMesh/NavMeshDescription.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <ThirdParty/Recast/DetourNavMeshQuery.h>
#include <ThirdParty/Recast/DetourPathCorridor.h>

class ezRecastWorldModule;
class ezPhysicsWorldModuleInterface;

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcAgentComponentManager : public ezComponentManager<class ezRcAgentComponent, ezBlockStorageType::FreeList>
{
  typedef ezComponentManager<class ezRcAgentComponent, ezBlockStorageType::FreeList> SUPER;

public:
  ezRcAgentComponentManager(ezWorld* pWorld);
  ~ezRcAgentComponentManager();

  virtual void Initialize() override;

  ezRecastWorldModule* GetRecastWorldModule() const { return m_pWorldModule; }

  void Update(const ezWorldModule::UpdateContext& context);

  ezPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
  ezRecastWorldModule* m_pWorldModule = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezRcAgentComponent : public ezRcComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezRcAgentComponent, ezRcComponent, ezRcAgentComponentManager);

public:
  ezRcAgentComponent();
  ~ezRcAgentComponent();

  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  void Update();


  void SetTargetPosition(const ezVec3& vPos);

  ezResult FindNavMeshPolyAt(ezVec3& inout_vPosition, dtPolyRef& out_PolyRef) const;

  ezVec3 ComputeSteeringDirection(float fMaxDistance);

  bool HasReachedPosition(const ezVec3& pos, float fMaxDistance) const;
  bool HasReachedGoal(float fMaxDistance) const;
  bool IsPositionVisible(const ezVec3& pos) const;

  void RenderPathCorridorPosition();
  void RenderPathCorridor();
  void VisualizeCurrentPath();
  void VisualizeTargetPosition();

  //////////////////////////////////////////////////////////////////////////
  // Properties
 
  float m_fWalkSpeed = 4.0f;
  ezUInt8 m_uiCollisionLayer = 0;

  /// \todo Expose and use
  float m_fRadius = 0.2f;
  float m_fHeight = 1.0f;

protected:
  virtual void OnSimulationStarted() override;

  bool Init();
  ezResult RecomputePathCorridor();
  ezResult PlanNextSteps();

  ezTime m_tLastUpdate;

  bool m_bInitialized = false;
  bool m_bHasPath = false;
  bool m_bHasValidCorridor = false;
  ezUniquePtr<dtNavMeshQuery> m_pQuery;
  ezUniquePtr<dtPathCorridor> m_pCorridor;

  dtPolyRef m_startPoly = -1;
  dtPolyRef m_endPoly = -1;
  ezInt32 m_iPathCorridorLength;
  ezDynamicArray<dtPolyRef> m_PathCorridor;
  ezVec3 m_vStartPosition;
  ezVec3 m_vEndPosition;

  // path following
  ezInt32 m_iFirstNextStep = 0;
  ezInt32 m_iNumNextSteps = 0;
  ezVec3 m_vNextSteps[16];
  ezUInt8 m_uiStepFlags[16];
  dtPolyRef m_StepPolys[16];
};
