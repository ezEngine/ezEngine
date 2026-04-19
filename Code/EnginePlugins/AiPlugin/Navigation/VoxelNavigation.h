#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Vec3.h>

class ezVoxelGrid;
class ezDebugRendererContext;

/// A* pathfinding through an ezVoxelGrid.
///
/// Uses 26-connected neighbors (including diagonals) for smooth paths.
/// After finding a raw voxel path, applies line-of-sight string-pulling
/// to remove unnecessary waypoints.
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

  void SetVoxelGrid(const ezVoxelGrid* pGrid);

  /// Sets the dynamic obstacle grid to consult during pathfinding.
  ///
  /// If set, both the static and dynamic grids are checked when determining whether
  /// a voxel is passable. Pass nullptr to clear.
  void SetDynamicObstacleGrid(const ezVoxelGrid* pDynGrid);

  /// Computes a path from vStart to vTarget through free voxels.
  ///
  /// uiMaxIterations limits the A* node expansion to prevent frame stalls.
  State FindPath(const ezVec3& vStart, const ezVec3& vTarget, ezUInt32 uiMaxIterations = 10000);

  const ezDynamicArray<ezVec3>& GetWaypoints() const { return m_Waypoints; }

  ezUInt32 GetCurrentWaypointIndex() const { return m_uiCurrentWaypoint; }

  /// Advances to the next waypoint. Returns true if there are more waypoints.
  bool AdvanceWaypoint();

  /// Returns the next waypoint to move towards.
  ezVec3 GetNextWaypoint() const;

  /// Returns true if the end of the path has been reached.
  bool IsPathComplete() const;

  void CancelNavigation();

  State GetState() const { return m_State; }

  /// Draws the path as a line strip.
  void DebugDrawPath(const ezDebugRendererContext& context, const ezColor& color) const;

private:
  void SmoothPath(ezDynamicArray<ezVec3>& inout_waypoints) const;

  /// Returns true if the voxel at coord is solid in either the static or dynamic grid.
  bool IsSolidVoxel(const ezVec3I32& coord) const;

  /// Ray-marches between two voxel coordinates and returns true if no solid voxel blocks the line.
  bool IsClearLine(const ezVec3I32& vStart, const ezVec3I32& vGoal) const;

  const ezVoxelGrid* m_pGrid = nullptr;
  const ezVoxelGrid* m_pDynamicGrid = nullptr;
  State m_State = State::Idle;
  ezDynamicArray<ezVec3> m_Waypoints;
  ezUInt32 m_uiCurrentWaypoint = 0;
};
