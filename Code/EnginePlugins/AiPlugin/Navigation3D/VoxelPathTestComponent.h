#pragma once

#include <AiPlugin/AiPluginDLL.h>
#include <AiPlugin/Navigation3D/VoxelNavigation.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>

using ezAiVoxelPathTestComponentManager = ezComponentManagerSimple<class ezAiVoxelPathTestComponent, ezComponentUpdateType::WhenSimulating>;

/// Used to test path-finding through voxel grids.
///
/// The component takes a reference to another game object as the destination and recomputes a path
/// towards it every frame using ezAiVoxelNavigation. The path can be visualized, coloring segments
/// that go through a voxel grid differently from straight-line segments that cross free space not
/// covered by any grid.
///
/// Repathing every frame is intentionally wasteful - this is a debugging aid, not something to run
/// in a shipping scene.
///
/// This component should be used in the editor, to test whether the scene's voxel grids produce the
/// desired paths.
class EZ_AIPLUGIN_DLL ezAiVoxelPathTestComponent : public ezComponent
{
  EZ_DECLARE_COMPONENT_TYPE(ezAiVoxelPathTestComponent, ezComponent, ezAiVoxelPathTestComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // ezComponent

public:
  virtual void SerializeComponent(ezWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(ezWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  //  ezAiVoxelPathTestComponent

public:
  ezAiVoxelPathTestComponent();
  ~ezAiVoxelPathTestComponent();

  void SetPathEndReference(const char* szReference); // [ property ]
  void SetPathEnd(ezGameObjectHandle hObject);

  /// Render the smoothed path, i.e. the one actually used for navigation.
  bool m_bVisualizeSmoothedPath = true; // [ property ]

  /// Render text describing the path search result.
  bool m_bVisualizePathState = true; // [ property ]

  /// Passed through to ezAiVoxelNavigation::FindPath().
  float m_fSearchMargin = 5.0f; // [ property ]

  /// Passed through to ezAiVoxelNavigation::FindPath().
  ezUInt32 m_uiMaxIterationsPerHop = 10000; // [ property ]

  /// Passed through to ezAiVoxelNavigation::FindPath().
  ezUInt32 m_uiMaxHops = 16; // [ property ]

protected:
  void Update();

  ezGameObjectHandle m_hPathEnd;
  ezAiVoxelNavigation m_Navigation;

private:
  const char* DummyGetter() const { return nullptr; }
};
