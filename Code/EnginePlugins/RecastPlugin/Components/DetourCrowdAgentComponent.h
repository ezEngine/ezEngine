#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/Components/AgentSteeringComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class ezDetourCrowdWorldModule;
class ezDetourCrowdAgentComponent;
struct ezDetourCrowdAgentParams;

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezDetourCrowdAgentComponentManager : public ezComponentManager<ezDetourCrowdAgentComponent, ezBlockStorageType::FreeList>
{
  using SUPER = ezComponentManager<ezDetourCrowdAgentComponent, ezBlockStorageType::FreeList>;

public:
  explicit ezDetourCrowdAgentComponentManager(ezWorld* pWorld);
  ~ezDetourCrowdAgentComponentManager();

  virtual void Initialize() override;

  ezDetourCrowdWorldModule* GetDetourCrowdWorldModule() const { return m_pDetourCrowdModule; }

private:
  void Update(const ezWorldModule::UpdateContext& ctx);

  ezUInt32 m_uiNextOwnerId = 1;
  ezDetourCrowdWorldModule* m_pDetourCrowdModule = nullptr;
};

//////////////////////////////////////////////////////////////////////////

class EZ_RECASTPLUGIN_DLL ezDetourCrowdAgentComponent : public ezAgentSteeringComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDetourCrowdAgentComponent, ezAgentSteeringComponent, ezDetourCrowdAgentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

protected:
  virtual void SerializeComponent(ezWorldWriter& stream) const override;
  virtual void DeserializeComponent(ezWorldReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // ezAgentSteeringComponent

public:
  ezDetourCrowdAgentComponent();
  ~ezDetourCrowdAgentComponent();

  virtual void SetTargetPosition(const ezVec3& vPosition) override;
  virtual ezVec3 GetTargetPosition() const override { return m_vTargetPosition; }
  virtual void ClearTargetPosition() override;
  virtual ezAgentPathFindingState::Enum GetPathToTargetState() const override { return m_PathToTargetState; }

  //////////////////////////////////////////////////////////////////////////
  // Properties
protected:
  float m_fRadius = 0.3f; // [ property ]
  float m_fHeight = 1.8f; // [ property ]
  float m_fMaxSpeed = 3.5f; // [ property ]
  float m_fMaxAcceleration = 3.5f; // [property]
  float m_fStoppingDistance = 1.0f; // [property]
  ezAngle m_MaxAngularSpeed = ezAngle::MakeFromDegree(360.0f); // [property]

public:
  float GetRadius() const { return m_fRadius; }
  float GetHeight() const { return m_fHeight; }
  float GetMaxSpeed() const { return m_fMaxSpeed; }
  float GetMaxAcceleration() const { return m_fMaxAcceleration; }
  float GetStoppingDistance() const { return m_fStoppingDistance; }
  ezAngle GetMaxAngularSpeed() const { return m_MaxAngularSpeed; }

  void SetRadius(float fRadius);
  void SetHeight(float fHeight);
  void SetMaxSpeed(float fMaxSpeed);
  void SetMaxAcceleration(float fMaxAcceleration);
  void SetStoppingDistance(float fStoppingDistance);
  void SetMaxAngularSpeed(ezAngle maxAngularSpeed);

  ezVec3 GetVelocity() const { return m_vVelocity; }
  ezAngle GetAngularSpeed() const { return m_AngularSpeed; }

  //////////////////////////////////////////////////////////////////////////
  // Other
public:
  void FillAgentParams(ezDetourCrowdAgentParams& out_params) const;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  void RotateTowardsDirection(ezQuat& inout_qCurrentRot, const ezVec3& vTargetDir, ezAngle& out_angularSpeed) const;

  void SyncTransform(const ezVec3& vPosition, const ezVec3& vVelocity, bool bTeleport);

  ezUInt8 m_uiTargetDirtyBit : 1;
  ezUInt8 m_uiSteeringFailedBit : 1;
  ezUInt8 m_uiErrorBit : 1;
  ezUInt8 m_uiParamsDirtyBit : 1;

  ezInt32 m_iAgentId = -1;
  ezUInt32 m_uiOwnerId = 0;
  ezVec3 m_vVelocity;
  ezAngle m_AngularSpeed;
  ezComponentHandle m_hCharacterController;
  ezVec3 m_vTargetPosition;
  ezEnum<ezAgentPathFindingState> m_PathToTargetState;
};
