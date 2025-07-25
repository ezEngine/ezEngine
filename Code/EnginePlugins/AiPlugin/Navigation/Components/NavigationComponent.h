#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/NavMeshQuery.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <AiPlugin/Navigation/Steering.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

EZ_DECLARE_FLAGS(ezUInt32, ezAiNavigationDebugFlags, PrintState, VisPathCorridor, VisPathLine, VisTarget);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiNavigationDebugFlags);

/// \brief Describes the different states a navigating object may be in.
struct ezAiNavigationComponentState
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Idle,    ///< Currently not navigating.
    Moving,  ///< Moving or waiting for a path to be computed.
    Turning,
    Falling, ///< High up above the ground, falling downwards.
    Fallen,  ///< Was high up, now reached the ground. May happen if spawned in air, otherwise should never happen, so this is a kind of error state.
    Failed,  ///< Path could not be found, either because start position is invalid (off mesh) or destination is not reachable.

    Default = Idle
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiNavigationComponentState);

using ezAiNavigationComponentManager = ezComponentManagerSimple<class ezAiNavigationComponent, ezComponentUpdateType::WhenSimulating>;

/// \brief Adds functionality to navigate on a navmesh.
///
/// Call SetDestination() to have the component move the parent game object along a path towards the goal.
/// Call GetState() to query whether it is moving and how.
///
/// This component is still very much work-in-progress. Things that need improvement:
///   * The state reporting is still limited, there is no distinction between failure states (invalid start position, target position, partial path)
///   * The 'destination reached' implementation is quite hacky.
///   * There is no way to stop navigating, but come to a stop smoothly (slowing down).
///   * There is no avoidance of dynamic obstacles (other creatures) whatsoever. They will just pass through each other.
///   * It is not designed to be pushed around dynamically. There is no physics character controller use to prevent it from being pushed into walls.
///   * If it somehow leaves the navmesh area, it just fails, there is no recovery mechanism.
class EZ_AIPLUGIN_DLL ezAiNavigationComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiNavigationComponent, ezComponent, ezAiNavigationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  //  ezAiNavMeshPathTestComponent

public:
  ezAiNavigationComponent();
  ~ezAiNavigationComponent();

  /// \brief Sets the target position to reach.
  ///
  /// If bAllowPartialPath is false, and a complete path can't be found (too far or simply not reachable),
  /// the 'Failed' state is used.
  /// Otherwise the 'Moving' state indicates that the character is navigating.
  void SetDestination(const ezVec3& vGlobalPos, bool bAllowPartialPath); ///< [ scriptable ]

  /// \brief Can be called at any time to stop moving.
  void CancelNavigation();                           ///< [ scriptable ]

  void StopWalking(float fWithinDistance);           ///< [ scriptable ]

  void TurnTowards(const ezVec2& vGlobalPos);        ///< [ scriptable ]

  ezHashedString m_sNavmeshConfig;                   ///< [ property ] Which navmesh to walk on.
  ezHashedString m_sPathSearchConfig;                ///< [ property ] What constraints there are for walking on the navmesh.

  float m_fReachedDistance = 1.0f;                   ///< [ property ] The distance at which the destination is considered to be reached.
  float m_fSpeed = 5.0f;                             ///< [ property ] The target speed to reach.
  float m_fFootRadius = 0.15f;                       ///< [ property ] The footprint to determine whether the character is standing on solid ground.
  ezUInt32 m_uiCollisionLayer = 0;                   ///< [ property ] The physics collision layer for determining what ground one can stand on.
  float m_fFallHeight = 0.7f;                        ///< [ property ] If there is more distance below the character than this, it is considered to be falling.
  float m_fAcceleration = 3.0f;                      ///< [ property ] How fast to gain speed.
  float m_fDecceleration = 8.0f;                     ///< [ property ] How fast to brake.

  ezBitflags<ezAiNavigationDebugFlags> m_DebugFlags; ///< [ property ] What aspects of the navigation to visualize.

  /// \brief Returns the current navigation state.
  ezEnum<ezAiNavigationComponentState> GetState() const { return m_State; } ///< [ scriptable ]


  /// \brief Checks whether the area around the given point is loaded and thus queries would succeed.
  ///
  /// If the area is not fully loaded, the function returns false.
  /// In this case, queries in that area will probably fail and should be delayed to a later point,
  /// since the navmesh first has to be generated.
  bool EnsureNavMeshSectorAvailable(const ezVec3& vCenter, float fRadius); ///< [ scriptable ]

  /// \brief Attempts to find a random point on the navmesh. The circle limits which navmesh polygons are visited.
  ///
  /// The result may be outside the circle, if the circle overlaps with a large navmesh polygon.
  bool FindRandomPointAroundCircle(const ezVec3& vCenter, float fRadius, ezVec3& out_vPoint); ///< [ scriptable ]

  bool RaycastNavMesh(const ezVec3& vStart, const ezVec3& vDirection, float fDistance, ezVec3& out_vPoint, float& out_fDistance);

  ezVec3 GetSteeringPosition() const; ///< [ scriptable ]
  ezQuat GetSteeringRotation() const; ///< [ scriptable ]

protected:
  void Update();
  void Steer(ezTransform& transform, float tDiff);
  void Turn(ezTransform& transform, float tDiff);
  void PlaceOnGround(ezTransform& transform, float tDiff);
  bool PrepareQueryObject();

  ezAiNavmeshQuery m_Query;
  ezEnum<ezAiNavigationComponentState> m_State;
  ezAiSteering m_Steering;
  ezAiNavigation m_Navigation;
  float m_fFallSpeed = 0.0f;
  bool m_bAllowPartialPath = false;
  bool m_bApplySteering = true;
  ezUInt8 m_uiSkipNextFrames = 0;
  float m_fStopWalkDistance = ezMath::HighValue<float>();
  ezVec2 m_vTurnTowardsPos = ezVec2::MakeZero();

  ezVec3 m_vSteerPosition;
  ezQuat m_qSteerRotation;

private:
  const char* DummyGetter() const { return nullptr; }
};
