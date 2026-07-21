#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation3D/VoxelNavigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Math/Angle.h>

EZ_DECLARE_FLAGS(ezUInt32, ezAiVoxelNavigationDebugFlags, PrintState, VisPath);
EZ_DECLARE_REFLECTABLE_TYPE(EZ_AIPLUGIN_DLL, ezAiVoxelNavigationDebugFlags);

/// Describes the different states a voxel-navigating object may be in.
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
/// a 3D A* path through free voxels and steers the game object along that path.
///
/// Tracks two separate positions. The path position is the ground truth: it always lies exactly
/// on the computed path (which is guaranteed clear - A*'d and string-pulled through free voxels)
/// and advances along it by arc length every frame; it is only ever voxel-validated once, when a
/// path is computed (SetDestination()/SetDestinationDirect()), never at runtime while moving.
/// The steering position is the visual position actually applied to the game object: it free-form
/// steers nose-first towards a look-ahead point on the path (LookAheadDistance) rather than
/// snapping onto waypoints or the path position, and is explicitly not voxel-validated - it may
/// lag or stray from the path, but is pulled back smoothly (CorridorCorrectionRate) once it
/// strays further than MaxPathOffset (a "path corridor").
///
/// Because the ground truth can never leave the cleared path, following it can never get stuck:
/// the Failed state is only reachable from SetDestination()/SetDestinationDirect() failing to find
/// a path at all (unreachable target), never from runtime movement.
///
/// Turning is rate-limited (MaxAngularSpeed) and speed is reduced while turning sharply and while
/// approaching the end of the path (Speed/Acceleration/Deceleration).
///
/// The object generally stays upright (local +Z towards world +Z); while turning it banks/tilts
/// into the curve (BankAmount, clamped by MaxBankAngle), relaxing back to level once it stops
/// turning.
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

  /// Sets the target position to fly straight towards, bypassing pathfinding and occlusion checks
  /// entirely - it just turns and moves in a straight line, using the same speed/acceleration/
  /// turning behavior as regular path-following.
  ///
  /// Useful for emergency recovery (e.g. the object ended up inside an occupied voxel and regular
  /// SetDestination() fails with InvalidStartPosition) or any other case where flying straight to a
  /// known-clear point is preferable to pathfinding. Combine with GetValidCellNearby() to first find
  /// a safe point to recover to.
  void SetDestinationDirect(const ezVec3& vGlobalPos); ///< [ scriptable ]

  /// Stops all navigation.
  void CancelNavigation(); ///< [ scriptable ]

  /// Returns the current navigation state.
  ezEnum<ezAiVoxelNavigationComponentState> GetState() const { return m_State; } ///< [ scriptable ]

  void SetNavigationTargetReference(const char* szReference);                    // [ property ]
  void SetNavigationTarget(ezGameObjectHandle hObject);                          ///< [ scriptable ]

  float m_fSpeed = 5.0f;                                                         ///< [ property ] Target movement speed.
  float m_fAcceleration = 3.0f;                                                  ///< [ property ] How fast to gain speed.
  float m_fDeceleration = 8.0f;                                                  ///< [ property ] How fast to brake.
  float m_fReachedDistance = 1.0f;                                               ///< [ property ] Distance at which destination is considered reached.
  bool m_bApplySteering = true;                                                  ///< [ property ] Whether to apply movement to the game object.
  float m_fLookAheadDistance = 3.0f;                                             ///< [ property ] How far ahead along the path to steer towards, instead of snapping onto waypoints.
  float m_fMaxPathOffset = 3.0f;                                                 ///< [ property ] Max distance the visual position may stray from the path before being pulled back.
  float m_fCorridorCorrectionRate = 8.0f;                                        ///< [ property ] How fast the visual position is pulled back into the corridor once it exceeds MaxPathOffset.
  ezAngle m_MaxAngularSpeed = ezAngle::MakeFromDegree(180.0f);                   ///< [ property ] Maximum turning speed, used both while path-following and by TurnTowards().
  float m_fBankAmount = 3.0f;                                                    ///< [ property ] How strongly to bank/tilt into turns while path-following. 0 disables banking; negative flips the bank direction.
  ezAngle m_MaxBankAngle = ezAngle::MakeFromDegree(30.0f);                       ///< [ property ] Maximum bank/tilt angle while turning.

  ezBitflags<ezAiVoxelNavigationDebugFlags> m_DebugFlags;                        ///< [ property ]

  /// Rotates the object to face vDirection, turning at most by MaxAngularSpeed this frame.
  ///
  /// Independent of any active path-following (typically used while Idle, e.g. to face a
  /// direction before deciding to call SetDestination()). If a path is currently being followed,
  /// this has no lasting effect: the path-following steering runs later in the same frame and
  /// will overwrite the rotation again next update.
  ///
  /// Returns the remaining angle to face vDirection exactly (zero once fully turned), so script
  /// logic can decide e.g. whether it is facing closely enough to start moving now.
  ezAngle TurnTowards(const ezVec3& vDirection); ///< [ scriptable ]

  /// Peek-only companion to TurnTowards(): how much the object would have to turn to face
  /// vDirection, without changing anything.
  ezAngle GetTurnAngleTowards(const ezVec3& vDirection) const; ///< [ scriptable ]

  /// Returns true if the component is currently navigating along a path.
  bool IsNavigating() const { return m_State == ezAiVoxelNavigationComponentState::Moving; } ///< [ scriptable ]

  /// Returns true if the component is navigating and the remaining path distance has entered the
  /// braking distance for Speed (i.e. it would start slowing down this frame if not redirected).
  ///
  /// Useful for objects that should keep moving continuously (never actually decelerate to a
  /// stop): call SetDestination()/SetNavigationTarget() with a new target once this turns true,
  /// instead of waiting for IsNavigating() to become false.
  bool IsApproachingDestination() const; ///< [ scriptable ]

  /// Attempts to find a random navigable point within a sphere around vCenter.
  ///
  /// A sampled point counts as navigable if it is a free voxel in its covering grid, or lies
  /// outside all grids entirely (space not covered by any grid is treated as free, same convention
  /// as pathfinding). Returns false if the voxel grid is not ready or no navigable point was found
  /// after uiMaxAttempts random samples.
  bool FindRandomPointAroundSphere(const ezVec3& vCenter, float fRadius, ezUInt32 uiMaxAttempts, ezVec3& out_vPoint); ///< [ scriptable ]

  /// Finds a navigable (free) point at or near vStart, preferring points that are physically closer
  /// to vStart. If vStart itself is already free (or not covered by any voxel grid at all), returns
  /// it unchanged. Otherwise searches for the closest free voxel within fSearchRadius.
  ///
  /// Intended for recovering from an occupied start position - e.g. combine with
  /// SetDestinationDirect() to move out of a voxel that turned solid - but can also be used to
  /// nudge an arbitrary destination onto a valid cell before calling SetDestination().
  ///
  /// Returns false if the voxel grid is not ready, or no free voxel was found within fSearchRadius.
  bool GetValidCellNearby(const ezVec3& vStart, float fSearchRadius, ezVec3& out_vPoint) const; ///< [ scriptable ]

protected:
  void Update();
  void MoveAlongPath(float fTimeDiff);
  void DecelerateToStop(float fTimeDiff);

  /// Turns m_qSteerRotation towards vDesiredDir (see ezAiSteeringUtils::TurnTowards - this also
  /// re-levels any existing roll), then banks/tilts it into the turn by up to MaxBankAngle,
  /// smoothed via m_BankAngle so it relaxes back to level once the turn ends.
  void SteerAndBank(const ezVec3& vDesiredDir, float fTimeDiff);

  /// Distance needed to brake from fSpeed down to zero at m_fDeceleration.
  float ComputeBrakingDistance(float fSpeed) const;

  /// Approximate remaining distance along the current path, or 0 if not navigating.
  float GetRemainingDistance() const;

  /// Pulls vCandidate back towards m_vPathPosition if it is further away than m_fMaxPathOffset,
  /// smoothly (rate m_fCorridorCorrectionRate) rather than snapping - the "path corridor" that
  /// keeps the free-form visual position from straying arbitrarily far from the path.
  ezVec3 ApplyCorridorClamp(const ezVec3& vCandidate, float fTimeDiff) const;

  ezAiVoxelNavigation m_Navigation;
  ezEnum<ezAiVoxelNavigationComponentState> m_State;
  ezGameObjectHandle m_hNavigationTarget;

  ezVec3 m_vVelocity = ezVec3::MakeZero();
  ezVec3 m_vSteerPosition = ezVec3::MakeZero();
  ezQuat m_qSteerRotation = ezQuat::MakeIdentity();

  ezVec3 m_vPathPosition = ezVec3::MakeZero(); ///< Ground truth: always on the path, always in a free voxel.
  float m_fPathSpeed = 0.0f;                   ///< Accel/decel-smoothed scalar speed for m_vPathPosition.
  bool m_bPathPositionInitialized = false;     ///< Not serialized; re-snapped to a valid voxel on first navigation each sim run.

  float m_fVoxelSize = 0.5f;                   ///< Cached voxel size of the grid being navigated; refreshed on each SetDestination(). Not serialized. The initial value is only a fallback used before the first successful path.

  /// Warm-up counter: Update() early-returns while this is > 0 (set in OnSimulationStarted()). Delays
  /// the first few updates after simulation start so the object's initial transform has settled
  /// (spawn/prefab instantiation, physics placement) and the voxel grid has become ready before the
  /// component starts driving the transform and pathfinding.
  ezUInt8 m_uiSkipNextFrames = 0;

  ezVec3 m_vLastPathTargetPos = ezVec3::MakeZero();
  float m_fRepathCooldown = 0.0f;

  ezAngle m_BankAngle = ezAngle::MakeFromRadian(0.0f);

private:
  const char* DummyGetter() const { return nullptr; }
};
