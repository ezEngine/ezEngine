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
  void Initialize(const ezVec3U32& vDimensions, const ezVec3& vCenter, float fVoxelSize);

  /// Clears all voxel data (sets everything to free). Keeps allocated memory.
  void ClearData();

  /// Converts a world-space position to integer voxel coordinates.
  EZ_FORCE_INLINE ezVec3I32 WorldToCoord(const ezVec3& vWorldPos) const
  {
    const ezVec3 vLocal = (vWorldPos - m_vOrigin) * m_fInvVoxelSize;
    return {ezMath::FloorToInt(vLocal.x), ezMath::FloorToInt(vLocal.y), ezMath::FloorToInt(vLocal.z)};
  }

  /// Converts integer voxel coordinates to the world-space center of that voxel.
  EZ_FORCE_INLINE ezVec3 CoordToWorld(const ezVec3I32& vCoord) const
  {
    return m_vOrigin + ezVec3((vCoord.x + 0.5f) * m_fVoxelSize, (vCoord.y + 0.5f) * m_fVoxelSize, (vCoord.z + 0.5f) * m_fVoxelSize);
  }

  /// Returns true if the coordinate is inside the grid bounds.
  EZ_FORCE_INLINE bool IsCoordValid(const ezVec3I32& vCoord) const
  {
    return vCoord.x >= 0 && vCoord.y >= 0 && vCoord.z >= 0 &&
           (ezUInt32)vCoord.x < m_vDimensions.x &&
           (ezUInt32)vCoord.y < m_vDimensions.y &&
           (ezUInt32)vCoord.z < m_vDimensions.z;
  }

  /// Sets a voxel to occupied (true) or free (false).
  void SetVoxel(const ezVec3I32& vCoord, bool bSolid);

  /// Returns whether the voxel at the given coordinate is occupied.
  bool IsVoxelSet(const ezVec3I32& vCoord) const;

  /// Rasterizes a world-space triangle into the grid, setting (or clearing) every voxel it overlaps.
  ///
  /// Cost scales with the triangle's voxel-space bounding box, so rasterizing geometry that is large
  /// relative to the voxel size is expensive. Parts of the triangle outside the grid are ignored.
  void SetVoxelsOnTriangle(const ezVec3& v0, const ezVec3& v1, const ezVec3& v2, bool bSet = true);

  /// Ray-march visibility test between two grid-space coordinates.
  ///
  /// Returns true if no occupied voxel blocks the line of sight.
  bool CheckLineOfSight(const ezVec3I32& vStart, const ezVec3I32& vGoal) const;

  /// Returns the world-space AABB of the entire grid.
  ezBoundingBox GetAABB() const;

  /// Returns the approximate memory usage in bytes.
  ezUInt64 GetHeapMemoryUsage() const;

  /// Draws a debug visualization of occupied surface voxels.
  void DebugDraw(const ezDebugRendererContext& context, const ezColor& color) const;

  ezVec3U32 GetDimensions() const { return m_vDimensions; }
  float GetVoxelSize() const { return m_fVoxelSize; }
  const ezVec3& GetOrigin() const { return m_vOrigin; }
  const ezVec3& GetCenter() const { return m_vCenter; }

private:
  EZ_FORCE_INLINE ezUInt32 GetBlockIndex(ezUInt32 uiBlockX, ezUInt32 uiBlockY, ezUInt32 uiBlockZ) const
  {
    return uiBlockZ * (m_vNumBlocks.x * m_vNumBlocks.y) + uiBlockY * m_vNumBlocks.x + uiBlockX;
  }

  EZ_FORCE_INLINE static ezUInt32 GetBitIndex(ezUInt32 uiLocalX, ezUInt32 uiLocalY, ezUInt32 uiLocalZ)
  {
    return uiLocalZ * 16u + uiLocalY * 4u + uiLocalX;
  }

  ezVec3U32 m_vDimensions;
  ezVec3U32 m_vNumBlocks;

  float m_fVoxelSize = 1.0f;
  float m_fInvVoxelSize = 1.0f;
  ezVec3 m_vCenter = ezVec3::MakeZero();
  ezVec3 m_vOrigin = ezVec3::MakeZero();

  ezDynamicArray<ezUInt64> m_Blocks;
};
