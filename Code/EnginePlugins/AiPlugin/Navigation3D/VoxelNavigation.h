#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Types/Delegate.h>

class ezVoxelGrid;
class ezDebugRendererContext;

/// Called by ezAiVoxelNavigation to find all voxel grids overlapping a search box.
using ezAiVoxelGridFinder = ezDelegate<void(const ezBoundingBox& searchBox, ezDynamicArray<const ezVoxelGrid*>& out_grids)>;

/// A* pathfinding through one or more ezVoxelGrid instances.
///
/// Uses 6-connected (face) neighbors only - no diagonal moves. Diagonal movement would let a path
/// cut across a corner between two solid voxels that only touch edge-to-edge or corner-to-corner,
/// squeezing through a gap that isn't actually open.
/// After finding a raw voxel path, applies line-of-sight string-pulling
/// to remove unnecessary waypoints.
///
/// A single search may span multiple, disjoint voxel grids: pathfinding is always
/// performed within one grid at a time. If the target lies outside the grid that is
/// currently being searched, the search only looks for a way out of that grid (any
/// reachable voxel on its boundary), then continues from there in the next grid found
/// on the way to the target. Space that is not covered by any grid is assumed to be free
/// (no obstacles).
class EZ_AIPLUGIN_DLL ezAiVoxelNavigation
{
public:
  ezAiVoxelNavigation();
  ~ezAiVoxelNavigation();

  enum class State
  {
    Idle,
    PathFound,
    NoPathFound,
    InvalidStartPosition,
    InvalidTargetPosition,
  };

  /// Computes a path from vStart to vTarget, potentially through multiple voxel grids.
  ///
  /// gridFinder is called (potentially multiple times) to find the voxel grids relevant for the
  /// next hop of the search, given a search box around the current position and vTarget.
  /// fSearchMargin is added around that box.
  /// uiMaxIterationsPerHop limits the A* node expansion within a single grid, to prevent frame stalls.
  /// uiMaxHops limits how many grids the path may pass through, as a safety net against infinite loops.
  State FindPath(const ezVec3& vStart, const ezVec3& vTarget, const ezAiVoxelGridFinder& gridFinder,
    float fSearchMargin = 5.0f, ezUInt32 uiMaxIterationsPerHop = 10000, ezUInt32 uiMaxHops = 16);

  /// Sets a trivial, single-segment path straight from vStart to vTarget, bypassing pathfinding and
  /// occlusion checks entirely. Useful for emergency recovery (e.g. moving out of a voxel that
  /// turned solid) or any other case where a straight line is known to be fine.
  void SetDirectPath(const ezVec3& vStart, const ezVec3& vTarget);

  const ezDynamicArray<ezVec3>& GetWaypoints() const { return m_Waypoints; }

  /// One entry per segment of GetWaypoints() (GetWaypoints().GetCount() - 1 entries). True if the
  /// segment [i, i + 1] runs through a voxel grid, false if it is a straight line through free space.
  const ezDynamicArray<bool>& GetSegmentInsideGrid() const { return m_SegmentInsideGrid; }

  ezUInt32 GetCurrentWaypointIndex() const { return m_uiCurrentWaypoint; }

  /// Advances to the next waypoint. Returns true if there are more waypoints.
  bool AdvanceWaypoint();

  /// Returns the next waypoint to move towards.
  ezVec3 GetNextWaypoint() const;

  /// Returns true if the end of the path has been reached.
  bool IsPathComplete() const;

  /// Walks the remaining path polyline starting from vCurrentPos (which does not need to lie
  /// exactly on the path), accumulating distance up to fLookAheadDistance. Returns the point at
  /// that distance along the path, or the final waypoint if the remaining path is shorter.
  /// Returns vCurrentPos itself if the path is already complete.
  ///
  /// Pure query - does not affect GetCurrentWaypointIndex(). See AdvanceAlongPath() for the
  /// mutating equivalent.
  ezVec3 GetLookAheadPoint(const ezVec3& vCurrentPos, float fLookAheadDistance) const;

  /// Advances a position by fDistance along the remaining path polyline, starting from vCurrentPos
  /// (which does not need to lie exactly on the path). Unlike GetLookAheadPoint(), this consumes
  /// (advances past) any waypoints crossed along the way, i.e. GetCurrentWaypointIndex() moves
  /// forward as a side effect. Returns the resulting point, clamped at the final waypoint once the
  /// path end is reached, or vCurrentPos itself if the path is already complete.
  ezVec3 AdvanceAlongPath(const ezVec3& vCurrentPos, float fDistance);

  void CancelNavigation();

  State GetState() const { return m_State; }

  /// Draws the path as a line strip.
  void DebugDrawPath(const ezDebugRendererContext& context, const ezColor& color) const;

  /// Draws the path, coloring each segment by whether it runs inside a voxel grid or crosses free
  /// space in a straight line.
  void DebugDrawPathSegments(const ezDebugRendererContext& context, const ezColor& insideGridColor,
    const ezColor& freeSpaceColor) const;

private:
  /// Searches for a path from vStart to vTarget, with both positions required to be inside grid.
  ///
  /// out_waypoints receives the A* path after string-pulling smoothing.
  State FindPathToTarget(const ezVoxelGrid& grid, const ezVec3& vStart, const ezVec3& vTarget, ezUInt32 uiMaxIterations,
    ezDynamicArray<ezVec3>& out_waypoints) const;

  /// Searches for a path from vStart to any voxel on the boundary of grid, biased towards vRealTarget.
  ///
  /// vRealTarget may lie outside grid; it is only used to bias the search heuristic.
  /// otherGrids are checked to reject exit voxels that fall inside a solid voxel of another, overlapping
  /// grid (grids are not supposed to overlap, but this guards against it regardless).
  /// out_waypoints receives the A* path after string-pulling smoothing.
  State FindPathToExit(const ezVoxelGrid& grid, const ezVec3& vStart, const ezVec3& vRealTarget,
    ezArrayPtr<const ezVoxelGrid* const> otherGrids, ezUInt32 uiMaxIterations, ezDynamicArray<ezVec3>& out_waypoints) const;

  void SmoothPath(const ezVoxelGrid& grid, ezDynamicArray<ezVec3>& inout_waypoints) const;

  /// Shared arc-length walk used by GetLookAheadPoint() (peek, local index copy) and
  /// AdvanceAlongPath() (mutates m_uiCurrentWaypoint via inout_uiIndex). Walks forward from
  /// vCurrentPos along the path polyline starting at m_Waypoints[inout_uiIndex], accumulating
  /// distance up to fDistance, advancing inout_uiIndex across any waypoints crossed along the way.
  ezVec3 WalkPathForward(const ezVec3& vCurrentPos, float fDistance, ezUInt32& inout_uiIndex) const;

  /// Searches all coordinates within uiMaxRadius voxels of vCoord for the closest valid, non-solid
  /// coordinate. Used to recover a search that would otherwise fail immediately because its exact
  /// start voxel is solid or out of bounds (e.g. a moving object ending up inside a voxel that
  /// turned solid after it last pathed, or momentum having carried it slightly past one).
  static bool FindNearbyValidCoord(const ezVoxelGrid& grid, const ezVec3I32& vCoord, ezUInt32 uiMaxRadius, ezVec3I32& out_vValidCoord);

  State m_State = State::Idle;
  ezDynamicArray<ezVec3> m_Waypoints;
  ezDynamicArray<bool> m_SegmentInsideGrid;
  ezUInt32 m_uiCurrentWaypoint = 0;
};
