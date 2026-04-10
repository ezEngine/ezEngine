#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Vec3.h>

class ezDebugRendererContext;

/// Stores a 3D voxel grid using packed 4x4x4 bit blocks.
///
/// Each block is stored as a single ezUInt64 (64 bits = 4*4*4 voxels).
/// A set bit means the voxel is occupied (solid). A cleared bit means the voxel is free (passable).
///
/// The grid has a fixed resolution and is centered at a configurable world position.
/// Use WorldToCoord / CoordToWorld to convert between world space and voxel coordinates.
/// Use InjectBox / InjectSphere to mark voxels as occupied based on world-space shapes.
class EZ_AIPLUGIN_DLL ezVoxelGrid
{
public:
  ezVoxelGrid();
  ~ezVoxelGrid();

  /// Initializes the grid with the given resolution (in voxels).
  ///
  /// Each dimension is rounded up to a multiple of 4 internally.
  /// The grid is cleared to all-free after init.
  void Init(ezUInt32 uiDimX, ezUInt32 uiDimY, ezUInt32 uiDimZ);

  /// Clears all voxel data (sets everything to free). Keeps allocated memory.
  void ClearData();

  /// Sets the world-space center and per-voxel size. Call after Init.
  void SetWorldParameters(const ezVec3& vCenter, float fVoxelSize);

  /// Converts a world-space position to integer voxel coordinates.
  /// Returns false if the position is outside the grid.
  bool WorldToCoord(const ezVec3& vWorldPos, ezVec3I32& out_vCoord) const;

  /// Converts integer voxel coordinates to the world-space center of that voxel.
  ezVec3 CoordToWorld(const ezVec3I32& vCoord) const;

  /// Returns true if the coordinate is inside the grid bounds.
  bool IsCoordValid(const ezVec3I32& vCoord) const;

  /// Sets a voxel to occupied (true) or free (false).
  void SetVoxel(const ezVec3I32& vCoord, bool bSolid);

  /// Returns true if the voxel at the given coordinate is occupied.
  bool CheckVoxel(const ezVec3I32& vCoord) const;

  /// Marks all voxels overlapping the given world-space AABB as occupied.
  void InjectBox(const ezBoundingBox& box);

  /// Marks all voxels overlapping the given world-space sphere as occupied.
  void InjectSphere(const ezVec3& vCenter, float fRadius);

  /// Clears all voxels overlapping the given world-space AABB (sets to free).
  void SubtractBox(const ezBoundingBox& box);

  /// Ray-march visibility test between two world-space points.
  ///
  /// Returns true if no occupied voxel blocks the line of sight.
  bool IsVisible(const ezVec3& vObserver, const ezVec3& vSubject) const;

  /// Returns the world-space AABB of the entire grid.
  ezBoundingBox GetAABB() const;

  /// Returns the approximate memory usage in bytes.
  ezUInt64 GetMemoryUsage() const;

  /// Draws a debug visualization of occupied surface voxels.
  void DebugDraw(const ezDebugRendererContext& context, const ezColor& color) const;

  ezUInt32 GetDimX() const { return m_uiDimX; }
  ezUInt32 GetDimY() const { return m_uiDimY; }
  ezUInt32 GetDimZ() const { return m_uiDimZ; }
  float GetVoxelSize() const { return m_fVoxelSize; }
  const ezVec3& GetCenter() const { return m_vCenter; }
  bool IsInitialized() const { return !m_Blocks.IsEmpty(); }

private:
  ezUInt32 GetBlockIndex(ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 uiBlockZ) const;
  static ezUInt32 GetBitIndex(ezUInt32 uiLocalX, ezUInt32 uiLocalY, ezUInt32 uiLocalZ);

  bool IsVisibleCoord(const ezVec3I32& vStart, const ezVec3I32& vGoal) const;

  ezUInt32 m_uiDimX = 0;
  ezUInt32 m_uiDimY = 0;
  ezUInt32 m_uiDimZ = 0;
  ezUInt32 m_uiBlocksX = 0;
  ezUInt32 m_uiBlocksY = 0;
  ezUInt32 m_uiBlocksZ = 0;

  float m_fVoxelSize = 1.0f;
  float m_fInvVoxelSize = 1.0f;
  ezVec3 m_vCenter = ezVec3::MakeZero();
  ezVec3 m_vOrigin = ezVec3::MakeZero();

  ezDynamicArray<ezUInt64> m_Blocks;
};
