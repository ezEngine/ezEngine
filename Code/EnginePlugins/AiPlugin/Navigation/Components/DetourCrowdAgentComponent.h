#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/ArrayMap.h>

class ezDetourCrowdAgentComponent;

//////////////////////////////////////////////////////////////////////////

struct ezDetourCrowdAgentRotationMode
{
  using StorageType = ezUInt8;

  enum Enum
  {
    LookAtNextPathCorner,
    MatchVelocityDirection,

    Default = LookAtNextPathCorner
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezDetourCrowdAgentRotationMode);

//////////////////////////////////////////////////////////////////////////

class EZ_AIPLUGIN_DLL ezDetourCrowdAgentComponentManager : public ezComponentManager<ezDetourCrowdAgentComponent, ezBlockStorageType::FreeList>
{
  using SUPER = ezComponentManager<ezDetourCrowdAgentComponent, ezBlockStorageType::FreeList>;

public:
  explicit ezDetourCrowdAgentComponentManager(ezWorld* pWorld);
  ~ezDetourCrowdAgentComponentManager();

  virtual void Initialize() override;

private:
  void AsyncUpdate(const UpdateContext& ctx);
  void SyncTransforms(const UpdateContext& ctx);

  static void FillDtCrowdAgentParams(const ezDetourCrowdAgentComponent* pAgent, dtCrowdAgentParams& out_params);
  static dtCrowdAgent* TryGetValidDtAgent(dtCrowd* pDtCrowd, const ezDetourCrowdAgentComponent* pAgent);

  ezUInt32 m_uiNextUniqueId = 0;
  ezArrayMap<ezHashedString, dtCrowd*> m_CrowdPerNavMesh;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Implements navigation, path following and obstacle avoidance. Requires Recast navmesh.
///
/// This component provides the ability to navigate around the level (using Recast navmesh)
/// while avoiding other agents.
///
/// Call SetDestination() to command the agent to navigate to a position. Check HasDestination() to
/// determine whether the agent is still navigating. If pathfinding fails and AllowPartialPath was
/// false, m_uiSteeringFailedBit is set.
class EZ_AIPLUGIN_DLL ezDetourCrowdAgentComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezDetourCrowdAgentComponent, ezComponent, ezDetourCrowdAgentComponentManager);

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

  /// \brief Sets the position to navigate to.
  ///
  /// If the position is not on navmesh, the nearest point on navmesh (with some threshold)
  /// will be the actual target. You can get it by calling GetActualTargetPosition().
  void SetDestination(const ezVec3& vGlobalPos, bool bAllowPartialPath);
  ezVec3 GetDestination() const { return m_vDestination; }
  void CancelNavigation();
  bool HasDestination() const { return m_uiHasDestinationBit; }

  //////////////////////////////////////////////////////////////////////////
  // Properties
protected:
  float m_fRadius = 0.3f;                                      ///< [ property ]
  float m_fHeight = 1.8f;                                      ///< [ property ]
  float m_fMaxSpeed = 3.5f;                                    ///< [ property ]
  float m_fMaxAcceleration = 10.0f;                            ///< [ property ]
  float m_fStoppingDistance = 0.1f;                            ///< [ property ] If distance to the target is less than the stopping distance, the target is reached.
  ezAngle m_MaxAngularSpeed = ezAngle::MakeFromDegree(360.0f); ///< [ property ]
  ezEnum<ezDetourCrowdAgentRotationMode> m_RotationMode;       ///< [ property ]
  float m_fPushiness = 1.0f;                                   ///< [ property ] The agent will push other agents with lower pushiness and will get pushed by agents with higher pushiness.
  ezHashedString m_sNavmeshConfig;                             ///< [ property ] Which navmesh to walk on.

public:
  float GetRadius() const { return m_fRadius; }
  float GetHeight() const { return m_fHeight; }
  float GetMaxSpeed() const { return m_fMaxSpeed; }
  float GetMaxAcceleration() const { return m_fMaxAcceleration; }
  float GetStoppingDistance() const { return m_fStoppingDistance; }
  ezAngle GetMaxAngularSpeed() const { return m_MaxAngularSpeed; }
  /// \brief While GetDestination() returns the requested target position,
  /// this one will return the actual point on navmesh that the agent is trying to reach.
  ezVec3 GetActualDestination() const { return m_vActualDestination; }
  ezDetourCrowdAgentRotationMode::Enum GetRotationMode() const { return m_RotationMode; }
  float GetPushiness() const { return m_fPushiness; }

  void SetRadius(float fRadius);
  void SetHeight(float fHeight);
  void SetMaxSpeed(float fMaxSpeed);
  void SetMaxAcceleration(float fMaxAcceleration);
  void SetStoppingDistance(float fStoppingDistance);
  void SetMaxAngularSpeed(ezAngle maxAngularSpeed);
  void SetRotationMode(ezDetourCrowdAgentRotationMode::Enum rotationMode) { m_RotationMode = rotationMode; }
  void SetPushiness(float fPushiness);

  ezVec3 GetVelocity() const { return m_vVelocity; }

  //////////////////////////////////////////////////////////////////////////
  // Other
protected:
  ezUInt8 m_uiDestinationChangedBit : 1;
  ezUInt8 m_uiHasDestinationBit : 1;
  ezUInt8 m_uiSteeringFailedBit : 1;  ///< Set when pathfinding fails and AllowPartialPath is false.
  ezUInt8 m_uiParamsChangedBit : 1;
  ezUInt8 m_uiAllowPartialPathBit : 1;

  ezUInt32 m_uiCrowdId = 0;
  ezUInt32 m_uiAgentId = 0;
  ezUInt32 m_uiUniqueAgentId = 0;
  ezVec3 m_vVelocity = ezVec3::MakeZero();
  ezVec3 m_vDestination = ezVec3::MakeZero();
  ezVec3 m_vActualDestination = ezVec3::MakeZero();
};
