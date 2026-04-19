#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/VoxelGrid.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Containers/DynamicArray.h>

/// World module that manages a voxel grid for 3D navigation.
///
/// Maintains two separate grids: one for static world geometry (filled once at startup)
/// and one for dynamic obstacles (updated at runtime). The dynamic grid uses per-voxel
/// reference counts so overlapping obstacles do not incorrectly cancel each other out.
///
/// Access it via GetWorld()->GetOrCreateModule<ezAiVoxelWorldModule>().
class EZ_AIPLUGIN_DLL ezAiVoxelWorldModule final : public ezWorldModule
{
  EZ_DECLARE_WORLD_MODULE();
  EZ_ADD_DYNAMIC_REFLECTION(ezAiVoxelWorldModule, ezWorldModule);

public:
  ezAiVoxelWorldModule(ezWorld* pWorld);
  ~ezAiVoxelWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  /// Returns the static grid (world geometry, filled once at startup). Used by pathfinding.
  ezVoxelGrid* GetVoxelGrid() { return &m_StaticGrid; }
  const ezVoxelGrid* GetVoxelGrid() const { return &m_StaticGrid; }

  /// Returns the dynamic obstacle grid (reference-counted, updated at runtime).
  const ezVoxelGrid* GetDynamicGrid() const { return &m_DynamicGrid; }

  /// Returns true once the grid has been voxelized and is ready for pathfinding.
  bool IsReady() const { return m_bIsReady; }

  /// Voxelizes static world geometry from physics collision data using axis-aligned raycasts.
  ///
  /// Fires rays along all three axes through the grid to mark surface voxels as solid.
  /// Should be called once at startup or when static geometry changes.
  void VoxelizeWorld(ezUInt32 uiCollisionLayer);

  /// Injects a box obstacle into the dynamic grid.
  ///
  /// Uses per-voxel reference counting so multiple overlapping obstacles accumulate correctly.
  void InjectObstacle(const ezBoundingBox& box);

  /// Removes a previously injected box obstacle from the dynamic grid.
  ///
  /// Decrements per-voxel reference counts. Only clears a voxel once all obstacles
  /// that cover it have been removed, preventing one obstacle from clearing another's voxels.
  void RemoveObstacle(const ezBoundingBox& box);

  ezUInt32 m_uiResolutionX = 128;
  ezUInt32 m_uiResolutionY = 128;
  ezUInt32 m_uiResolutionZ = 64;
  float m_fVoxelSize = 0.5f;
  ezVec3 m_vGridCenter = ezVec3::MakeZero();
  ezUInt32 m_uiCollisionLayer = 0;

private:
  void Update(const UpdateContext& ctxt);

  ezVoxelGrid m_StaticGrid;
  ezVoxelGrid m_DynamicGrid;
  ezDynamicArray<ezUInt8> m_ObstacleCounts; ///< Per-voxel obstacle reference counts, indexed same as grid flat layout.

  bool m_bNeedsVoxelization = true;
  bool m_bIsReady = false;
  ezUInt32 m_uiUpdateDelay = 10;
};
