#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/VoxelNavigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

EZ_DECLARE_FLAGS(ezUInt32, ezAiVoxelNavigationDebugFlags, PrintState, VisPath, VisGrid);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiVoxelNavigationDebugFlags);

/// \brief Describes the different states a voxel-navigating object may be in.
struct ezAiVoxelNavigationComponentState
{
  using StorageType = ezUInt8;

  enum Enum
  {
    Idle,   ///< Currently not navigating.
    Moving, ///< Moving along a computed 3D path.
    Failed, ///< Path could not be found.

    Default = Idle
  };
};

EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiVoxelNavigationComponentState);

using ezAiVoxelNavigationComponentManager = ezComponentManagerSimple<class ezAiVoxelNavigationComponent, ezComponentUpdateType::WhenSimulating>;

/// Navigates a game object through 3D space using a voxel grid.
///
/// Suitable for flying creatures, underwater movement, or space navigation.
/// Call SetDestination() with a world-space target. The component computes
/// a 3D A* path through free voxels and moves the game object along that path.
class EZ_AIPLUGIN_DLL ezAiVoxelNavigationComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiVoxelNavigationComponent, ezComponent, ezAiVoxelNavigationComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // ezAiVoxelNavigationComponent

public:
  ezAiVoxelNavigationComponent();
  ~ezAiVoxelNavigationComponent();

  /// Sets the target position to navigate towards.
  void SetDestination(const ezVec3& vGlobalPos); ///< [ scriptable ]

  /// Stops all navigation.
  void CancelNavigation(); ///< [ scriptable ]

  /// Returns the current navigation state.
  ezEnum<ezAiVoxelNavigationComponentState> GetState() const { return m_State; } ///< [ scriptable ]

  void SetNavigationTargetReference(const char* szReference); // [ property ]
  void SetNavigationTarget(ezGameObjectHandle hObject); ///< [ scriptable ]

  float m_fSpeed = 5.0f;          ///< [ property ] Target movement speed.
  float m_fAcceleration = 3.0f;   ///< [ property ] How fast to gain speed.
  float m_fDeceleration = 8.0f;   ///< [ property ] How fast to brake.
  float m_fReachedDistance = 1.0f; ///< [ property ] Distance at which destination is considered reached.
  bool m_bApplySteering = true;    ///< [ property ] Whether to apply movement to the game object.

  ezBitflags<ezAiVoxelNavigationDebugFlags> m_DebugFlags; ///< [ property ]

  ezVec3 GetSteeringPosition() const; ///< [ scriptable ]
  ezQuat GetSteeringRotation() const; ///< [ scriptable ]

  /// Returns the current velocity vector.
  ezVec3 GetVelocity() const { return m_vVelocity; } ///< [ scriptable ]

  /// Returns true if the component is currently navigating along a path.
  bool IsNavigating() const { return m_State == ezAiVoxelNavigationComponentState::Moving; } ///< [ scriptable ]

  /// Returns the approximate remaining distance along the current path, or 0 if not navigating.
  float GetRemainingDistance() const; ///< [ scriptable ]

  /// Returns the number of waypoints in the current path.
  ezUInt32 GetWaypointCount() const; ///< [ scriptable ]

  /// Attempts to find a random navigable point within a sphere around vCenter.
  ///
  /// Returns false if the voxel grid is not ready or no free voxel was found after several attempts.
  bool FindRandomPointAroundSphere(const ezVec3& vCenter, float fRadius, ezVec3& out_vPoint); ///< [ scriptable ]

protected:
  void Update();
  void MoveTowardsWaypoint(float fTimeDiff);

  ezAiVoxelNavigation m_Navigation;
  ezEnum<ezAiVoxelNavigationComponentState> m_State;
  ezGameObjectHandle m_hNavigationTarget;

  ezVec3 m_vVelocity = ezVec3::MakeZero();
  ezVec3 m_vSteerPosition = ezVec3::MakeZero();
  ezQuat m_qSteerRotation = ezQuat::MakeIdentity();

  float m_fVoxelSize = 0.5f;
  ezUInt8 m_uiSkipNextFrames = 0;

private:
  const char* DummyGetter() const { return nullptr; }
};
