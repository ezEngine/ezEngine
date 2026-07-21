#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation3D/VoxelGrid.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/BoundingBox.h>

class ezAiVoxelGridComponent;

/// World module that manages the voxel grids used for 3D navigation.
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

  /// Returns true once the grid has been voxelized and is ready for pathfinding.
  bool IsReady() const { return m_bIsReady; }

  /// Finds all ezAiVoxelGridComponent instances whose bounds overlap the given AABB.
  void FindGridsInBox(const ezBoundingBox& box, ezDynamicArray<ezAiVoxelGridComponent*>& out_grids) const;

private:
  void Update(const UpdateContext& ctxt);

  bool m_bIsReady = false;

  /// Number of PostTransform updates to wait after simulation start before the first voxelization
  /// (and thus before IsReady() returns true). Gives the collision geometry - physics bodies and
  /// spawned/streamed objects - time to settle so the static grid rasterizes the final world state.
  ezUInt32 m_uiUpdateDelay = 10;
};
