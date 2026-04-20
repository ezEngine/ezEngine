#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Foundation/Math/BoundingBox.h>

using ezAiVoxelObstacleComponentManager = ezComponentManager<class ezAiVoxelObstacleComponent, ezBlockStorageType::Compact>;

/// Represents a dynamic obstacle in the voxel navigation grid.
///
/// When activated, injects its owner's bounding box into the voxel grid as occupied.
/// When deactivated, removes it. Useful for doors, moving platforms, or spawned barriers.
class EZ_AIPLUGIN_DLL ezAiVoxelObstacleComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiVoxelObstacleComponent, ezComponent, ezAiVoxelObstacleComponentManager);

public:
  ezAiVoxelObstacleComponent();
  ~ezAiVoxelObstacleComponent();

  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  /// Manually re-inject the obstacle, e.g. after moving it.
  void UpdateObstacle(); ///< [ scriptable ]

  /// Returns whether the obstacle is currently injected into the voxel grid.
  bool IsInjected() const { return m_bInjected; } ///< [ scriptable ]

  ezUInt32 m_uiCollisionLayer = 0; ///< [ property ]

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

private:
  void InjectIntoGrid();
  void RemoveFromGrid();

  ezBoundingBox m_LastInjectedBounds;
  bool m_bInjected = false;
};
