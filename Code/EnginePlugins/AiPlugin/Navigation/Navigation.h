#pragma once

#include <AiPlugin/Navigation/NavMesh.h>
#include <DetourNavMeshQuery.h>
#include <DetourPathCorridor.h>
#include <Foundation/Math/Angle.h>
#include <Foundation/Math/Vec3.h>

class ezDebugRendererContext;

/// \brief Aggregated data by ezAiNavigation that should be sufficient to implement a steering behavior.
struct ezAiSteeringInfo
{
  ezVec3 m_vNextWaypoint;
  float m_fDistanceToWaypoint = 0;
  float m_fArrivalDistance = ezMath::HighValue<float>();
  ezVec2 m_vDirectionTowardsWaypoint = ezVec2::MakeZero();
  ezAngle m_AbsRotationTowardsWaypoint = ezAngle::MakeZero();
  ezAngle m_MaxAbsRotationAfterWaypoint = ezAngle::MakeZero();
  // float m_fWaypointCorridorWidth = ezMath::HighValue<float>();
};

/// \brief Computes a path through a navigation mesh.
///
/// First call SetNavmesh() and SetQueryFilter().
///
/// When you need a path, call SetCurrentPosition() and SetTargetPosition() to inform the
/// system of the current position and desired target location.
/// Then call Update() once per frame to have it compute the path.
/// Call GetState() to figure out whether a path exists.
/// Use ComputeAllWaypoints() to get an entire path, e.g. for visualization.
/// For steering this is not necessary. Instead use ComputeSteeringInfo() to plan the next step.
/// Apply your steering behavior to your character as desired.
/// Keep calling SetCurrentPosition() and SetTargetPosition() to inform the ezAiNavigation of the
/// new state, and keep calling ComputeSteeringInfo() every frame for the updated path.
///
/// If the destination was reached, a completely different path should be computed, or the current
/// path should be canceled, call CancelNavigation().
/// To start a new path search, call SetTargetPosition() again (and Update() every frame).
class EZ_AIPLUGIN_DLL ezAiNavigation final
{
public:
  ezAiNavigation();
  ~ezAiNavigation();

  enum class State
  {
    Idle,
    StartNewSearch,
    InvalidCurrentPosition,
    InvalidTargetPosition,
    NoPathFound,
    PartialPathFound,
    FullPathFound,
    Searching,
  };

  static constexpr ezUInt32 MaxPathNodes = 64;
  static constexpr ezUInt32 MaxSearchNodes = MaxPathNodes * 8;

  State GetState() const { return m_State; }

  void Update();

  void CancelNavigation();

  void SetCurrentPosition(const ezVec3& vPosition);

  /// \brief Sets the desired target location and starts a path search.
  ///
  /// If \a bOptimizeWhenFound is true, the corridor is optimized (topology + visibility) once,
  /// as soon as the path search finishes. Use this to avoid the weird corridor shapes that the
  /// incremental (counter-based) optimization produces right after a repath. The path search is
  /// asynchronous, so the optimization cannot happen inside this call - it happens in a later
  /// Update() when the search completes.
  void SetTargetPosition(const ezVec3& vPosition, bool bOptimizeWhenFound = false);
  const ezVec3& GetTargetPosition() const;

  /// \brief Immediately optimizes the current path corridor (topology + visibility).
  ///
  /// Only has an effect when a path exists (GetState() == FullPathFound or PartialPathFound).
  /// This is the on-demand counterpart to SetTargetPosition()'s bOptimizeWhenFound flag.
  void OptimizeCurrentPath();

  /// \brief Checks whether \a vPosition lies inside the current path corridor.
  ///
  /// Returns true only if the point is directly above/below one of the corridor polygons and
  /// within \a fHeightTolerance of that polygon's surface. This is different from testing whether
  /// a point is on the navmesh at all, and different from a raycast: it answers "would moving to
  /// this position leave the planned corridor?". Returns false if no path exists.
  bool IsPointInPathCorridor(const ezVec3& vPosition, float fHeightTolerance = 0.5f) const;
  void SetNavmesh(ezAiNavMesh* pNavmesh);
  void SetQueryFilter(const dtQueryFilter& filter);

  void ComputeAllWaypoints(ezDynamicArray<ezVec3>& out_waypoints) const;

  void DebugDrawPathCorridor(const ezDebugRendererContext& context, ezColor tilesColor, float fPolyRenderOffsetZ = 0.1f);
  void DebugDrawPathLine(const ezDebugRendererContext& context, ezColor straightLineColor, float fLineRenderOffsetZ = 0.2f);
  void DebugDrawState(const ezDebugRendererContext& context, const ezVec3& vPosition) const;


  /// \brief Returns the height of the navmesh at the current position.
  float GetCurrentElevation() const;

  void ComputeSteeringInfo(ezAiSteeringInfo& out_info, const ezVec2& vForwardDir, float fMaxLookAhead = 5.0f);

  // in what radius / up / down distance navigation mesh polygons should be searched around a given position
  // this should relate to the character size, ie at least the character radius
  // otherwise a character that barely left the navmesh area may not know where it is, anymore
  float m_fPolySearchRadius = 0.5f;
  float m_fPolySearchUp = 1.5f;
  float m_fPolySearchDown = 1.5f;

  // when a path search is started, all tiles in a rectangle around the start and end point are loaded first
  // this is the amount to increase that rectangle size, to overestimate which sectors may be needed during the path search
  constexpr static float c_fPathSearchBoundary = 10.0f;


private:
  State m_State = State::Idle;

  ezVec3 m_vCurrentPosition = ezVec3::MakeZero();
  ezVec3 m_vTargetPosition = ezVec3::MakeZero();

  ezUInt8 m_uiCurrentPositionChangedBit : 1;
  ezUInt8 m_uiTargetPositionChangedBit : 1;
  ezUInt8 m_uiReinitQueryBit : 1;
  ezUInt8 m_uiOptimizeWhenFoundBit : 1;

  ezAiNavMesh* m_pNavmesh = nullptr;
  dtNavMeshQuery m_Query;
  const dtQueryFilter* m_pFilter = nullptr;
  dtPathCorridor m_PathCorridor;

  dtPolyRef m_PathSearchTargetPoly;
  ezVec3 m_vPathSearchTargetPos;

  ezUInt8 m_uiOptimizeTopologyCounter = 0;
  ezUInt8 m_uiOptimizeVisibilityCounter = 0;

  bool UpdatePathSearch();
};
