#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation/VoxelGrid.h>
#include <Core/World/WorldModule.h>

/// World module that manages a voxel grid for 3D navigation.
///
/// Provides the voxel grid to navigation components.
/// Can voxelize the world from physics collision geometry on demand.
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

  ezVoxelGrid* GetVoxelGrid() { return &m_VoxelGrid; }
  const ezVoxelGrid* GetVoxelGrid() const { return &m_VoxelGrid; }

  /// Returns true once the grid has been voxelized and is ready for pathfinding.
  bool IsReady() const { return m_bIsReady; }

  /// Triggers voxelization of the world using physics overlap tests.
  ///
  /// This iterates every voxel cell and does a box overlap test to determine occupancy.
  /// Expensive for large grids. Should be called once at startup or when the world changes significantly.
  void VoxelizeWorld(ezUInt32 uiCollisionLayer);

  /// Injects a box obstacle into the voxel grid.
  void InjectObstacle(const ezBoundingBox& box);

  /// Removes a box obstacle from the voxel grid.
  void RemoveObstacle(const ezBoundingBox& box);

  ezUInt32 m_uiResolutionX = 128;
  ezUInt32 m_uiResolutionY = 128;
  ezUInt32 m_uiResolutionZ = 64;
  float m_fVoxelSize = 0.5f;
  ezVec3 m_vGridCenter = ezVec3::MakeZero();
  ezUInt32 m_uiCollisionLayer = 0;

private:
  void Update(const UpdateContext& ctxt);

  ezVoxelGrid m_VoxelGrid;
  bool m_bNeedsVoxelization = true;
  bool m_bIsReady = false;
  ezUInt32 m_uiUpdateDelay = 10;
};
